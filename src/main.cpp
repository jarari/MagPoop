#include "MathUtils.h"
#include "Utilities.h"
#include "half.h"
using namespace RE;

const F4SE::TaskInterface* taskInterface;
PlayerCharacter* p;

bool ValidateCollider(std::string name)
{
	if (name == "ar" || name == "pistol" || name == "banana" || name == "drum")
		return true;
	_MESSAGE("Error : Invalid collider type");
	return false;
}

int16_t GetVertexCount(NiAVObject* tri)
{
	return *(int16_t*)((uintptr_t)tri + 0x164);
}

NiAVObject* GetMagTri(NiAVObject* root)
{
	int16_t vertMax = 0;
	NiAVObject* mag = nullptr;
	Visit(root, [&](NiAVObject* obj) {
		if (obj->IsTriShape()) {
			int16_t vertCount = GetVertexCount(obj);
			if (vertCount > vertMax) {
				vertMax = vertCount;
				mag = obj;
			}
		}
		return false;
	});
	return mag;
}

NiPoint3 GetTriCenter(NiAVObject* tri)
{
	BSGraphics::TriShape* triShape = *(BSGraphics::TriShape**)((uintptr_t)tri + 0x148);
	BSGraphics::VertexDesc* vertexDesc = (BSGraphics::VertexDesc*)((uintptr_t)tri + 0x150);
	int16_t vertexCount = *(int16_t*)((uintptr_t)tri + 0x164);
	uint32_t vertexSize = vertexDesc->GetSize();
	uint32_t posOffset = vertexDesc->GetAttributeOffset(BSGraphics::Vertex::VA_POSITION);
	NiPoint3 ret;
	if (triShape && triShape->buffer08) {
		for (int v = 0; v < vertexCount; ++v) {
			uintptr_t posPtr = (uintptr_t)triShape->buffer08->rawVertexData + v * vertexSize + posOffset;
			NiPoint3 pos{ half_float::half_cast<float>(*(half_float::half*)(posPtr)), half_float::half_cast<float>(*(half_float::half*)(posPtr + 0x2)), half_float::half_cast<float>(*(half_float::half*)(posPtr + 0x4)) };
			ret = ret + pos;
		}
	}
	return ret / (float)vertexCount;
}

class AnimationGraphEventWatcher
{
public:
	typedef BSEventNotifyControl (AnimationGraphEventWatcher::*FnProcessEvent)(BSAnimationGraphEvent& evn, BSTEventSource<BSAnimationGraphEvent>* dispatcher);

	BSEventNotifyControl HookedProcessEvent(BSAnimationGraphEvent& evn, BSTEventSource<BSAnimationGraphEvent>* src)
	{
		Actor* a = (Actor*)((uintptr_t)this - 0x38);
		if (a->Get3D() && a->parentCell && a->gunState == GUN_STATE::kReloading) {
			if ((evn.animEvent == "countDownTick") && evn.argument.length() != 0) {
				std::string boneName;
				std::string collType = SplitString(evn.argument.c_str(), "|", boneName);
				std::string velX, velY, velZ;
				boneName = SplitString(boneName, "|", velX);
				velX = SplitString(velX, "|", velY);
				velY = SplitString(velY, "|", velZ);
				for (auto& c : collType) {
					c = tolower(c);
				}
				if (ValidateCollider(collType)) {
					//_MESSAGE("Mag drop on %s, collider type %s", boneName.c_str(), collType.c_str());
					//_MESSAGE("Vel %s %s %s", velX.c_str(), velY.c_str(), velZ.c_str());
					NiAVObject* node = BSUtilities::GetObjectByName(a->Get3D(), boneName, true, true);
					NiAVObject* mag = BSUtilities::GetObjectByName(a->Get3D(), "WeaponMagazine", true, true);
					if (node && mag) {
						MemoryManager mm = MemoryManager::GetSingleton();
						BSTempEffectDebris* magDebris = (BSTempEffectDebris*)mm.Allocate(sizeof(BSTempEffectDebris), 0, false);
						if (magDebris) {
							//_MESSAGE("Debris spawned %llx", magDebris);
							NiPoint3 pos = node->world.translate;
							NiPoint3 vel = NiPoint3(0, 0, -30.f);
							if (velX.length() > 0 && velY.length() > 0 && velZ.length() > 0) {
								vel.x = std::stof(velX);
								vel.y = std::stof(velY);
								vel.z = std::stof(velZ);
							}
							vel = GetRotationMatrix33(0, -a->data.angle.x, -a->data.angle.z) * vel;
							_MESSAGE("vel %f %f %f", vel.x, vel.y, vel.z);
							//_MESSAGE("Vel calculated %f %f %f", vel.x, vel.y, vel.z);
							bool isFP = false;
							if (a == p) {
								pos = pos + (*F4::ptr_PlayerAdjust);
								isFP = !p->Is3rdPersonVisible();
								//_MESSAGE("IsPlayer, isFP %d", isFP);
							}
							//_MESSAGE("pos %f %f %f", pos.x, pos.y, pos.z);
							/*BGSEquipIndex equipIndex;
							equipIndex.index = 0;
							TESAmmo* ammo = a->GetCurrentAmmo(equipIndex);*/
							//_MESSAGE("shellCasing %s", ammo->shellCasing.model.c_str());
							char buf[32];
							sprintf_s(buf, sizeof(buf), "Weapons\\MagColliders\\%s.nif", collType.c_str());
							//_MESSAGE("Fetching collider %s", buf);
							new (magDebris) BSTempEffectDebris(a->parentCell, 10.0f, buf, a, pos, node->world.rotate, vel, NiPoint3(), 1.0f, false, true, isFP);
							magDebris->IncRefCount();

							NiAVObject* magDebris3D = magDebris->Get3D();
							if (magDebris3D) {
								//_MESSAGE("magDebris3D found %llx", magDebris3D);
								//bhkNPCollisionObject* magColl = (bhkNPCollisionObject*)mag->collisionObject.get();
								//bhkNPCollisionObject* debrisColl = (bhkNPCollisionObject*)magDebris3D->collisionObject.get();
								NiAVObject* shellTri = nullptr;
								/*Visit(mag, [&](NiAVObject* obj) {
									if (obj->IsbhkNPCollisionObject()) {
										magColl = obj->IsbhkNPCollisionObject();
									} else if (obj->collisionObject.get()) {
										magColl = (bhkNPCollisionObject*)obj->collisionObject.get();
									}
									return false;
								});*/
								Visit(magDebris3D, [&](NiAVObject* obj) {
									if (obj->IsTriShape()) {
										shellTri = obj;
									}
									return false;
								});
								NiAVObject* targetMagTri = GetMagTri(node);
								if (targetMagTri) {
									NiCloningProcess cp{};
									cp.unk60 = 1;
									//_MESSAGE("Cloning mag");
									NiAVObject* clonedMagTri = (NiAVObject*)targetMagTri->CreateClone(cp);
									if (clonedMagTri) {
										magDebris3D->IsNode()->AttachChild(clonedMagTri, true);
										NiPoint3 center = GetTriCenter(clonedMagTri);
										clonedMagTri->local.translate = center * -1.f;
									}
								}

								/*if (magColl && debrisColl) {
									//_MESSAGE("Copy memebers");
									debrisColl->CopyMembers(magColl, cp);
									//_MESSAGE("Copy done");
								}*/

								if (shellTri) {
									//_MESSAGE("Hiding shell");
									shellTri->local.scale = 0.f;
								}
							}
						}
					} else {
						_MESSAGE("Error : Failed to find node!");
					}
				}
			}
		}
		FnProcessEvent fn = fnHash.at(*(uintptr_t*)this);
		return fn ? (this->*fn)(evn, src) : BSEventNotifyControl::kContinue;
	}

	void HookSink()
	{
		uintptr_t vtable = *(uintptr_t*)this;
		auto it = fnHash.find(vtable);
		if (it == fnHash.end()) {
			FnProcessEvent fn = SafeWrite64Function(vtable + 0x8, &AnimationGraphEventWatcher::HookedProcessEvent);
			fnHash.insert(std::pair<uintptr_t, FnProcessEvent>(vtable, fn));
		}
	}

protected:
	static std::unordered_map<uintptr_t, FnProcessEvent> fnHash;
};
std::unordered_map<uintptr_t, AnimationGraphEventWatcher::FnProcessEvent> AnimationGraphEventWatcher::fnHash;

void InitializePlugin()
{
	p = PlayerCharacter::GetSingleton();
	((AnimationGraphEventWatcher*)((uintptr_t)p + 0x38))->HookSink();
	uintptr_t ActorVtable = REL::Relocation<uintptr_t>{ Actor::VTABLE[3] }.address();
	((AnimationGraphEventWatcher*)(&ActorVtable))->HookSink();
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* a_f4se, F4SE::PluginInfo* a_info)
{
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= fmt::format(FMT_STRING("{}.log"), Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

#ifndef NDEBUG
	log->set_level(spdlog::level::trace);
#else
	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::warn);
#endif

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("%g(%#): [%^%l%$] %v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);

	a_info->infoVersion = F4SE::PluginInfo::kVersion;
	a_info->name = Version::PROJECT.data();
	a_info->version = Version::MAJOR;

	if (a_f4se->IsEditor()) {
		logger::critical(FMT_STRING("loaded in editor"));
		return false;
	}

	const auto ver = a_f4se->RuntimeVersion();
	if (ver < F4SE::RUNTIME_1_10_162) {
		logger::critical(FMT_STRING("unsupported runtime v{}"), ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
	F4SE::Init(a_f4se);

	taskInterface = F4SE::GetTaskInterface();
	const F4SE::MessagingInterface* message = F4SE::GetMessagingInterface();
	message->RegisterListener([](F4SE::MessagingInterface::Message* msg) -> void {
		if (msg->type == F4SE::MessagingInterface::kGameDataReady) {
			InitializePlugin();
		}
	});

	return true;
}
