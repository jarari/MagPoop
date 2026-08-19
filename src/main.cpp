#include "MathUtils.h"
#include "Utilities.h"
#include "half.h"
using namespace RE;

const F4SE::TaskInterface* taskInterface;
PlayerCharacter* p;
REL::Relocation<Setting*> fGunShellLifetime{ REL::ID{ 1487562, 1487562 } };

bool ValidateCollider(std::string name) {
	if (name == "ar" || name == "pistol" || name == "banana" || name == "drum")
		return true;
	_MESSAGE("Error : Invalid collider type");
	return false;
}

std::uint16_t GetVertexCount(NiAVObject* tri) {
	auto* triShape = tri ? tri->IsTriShape() : nullptr;
	return triShape ? triShape->numVertices : 0;
}

NiAVObject* GetMagTri(NiAVObject* root) {
	std::uint16_t vertMax = 0;
	NiAVObject* mag = nullptr;
	Visit(root, [&](NiAVObject* obj) {
		if (obj->IsTriShape()) {
			const auto vertCount = GetVertexCount(obj);
			if (vertCount > vertMax) {
				vertMax = vertCount;
				mag = obj;
			}
		}
		return false;
	});
	return mag;
}

NiPoint3 GetTriCenter(NiAVObject* tri) {
	auto* geometry = tri ? tri->IsTriShape() : nullptr;
	if (!geometry || geometry->numVertices == 0) {
		return {};
	}

	auto* rendererData = static_cast<BSGraphics::TriShape*>(geometry->rendererData);
	const auto vertexSize = geometry->vertexDesc.GetSize();
	const auto posOffset = geometry->vertexDesc.GetAttributeOffset(BSGraphics::Vertex::VA_POSITION);
	NiPoint3 ret{};
	if (rendererData && rendererData->vertexBuffer) {
		for (std::uint16_t v = 0; v < geometry->numVertices; ++v) {
			uintptr_t posPtr = reinterpret_cast<std::uintptr_t>(rendererData->vertexBuffer->data) + v * vertexSize + posOffset;
			NiPoint3 pos{ half_float::half_cast<float>(*(half_float::half*)(posPtr)), half_float::half_cast<float>(*(half_float::half*)(posPtr + 0x2)), half_float::half_cast<float>(*(half_float::half*)(posPtr + 0x4)) };
			ret = ret + pos;
		}
	}
	return ret / static_cast<float>(geometry->numVertices);
}

class AnimationGraphEventWatcher {
public:
	typedef BSEventNotifyControl (AnimationGraphEventWatcher::* FnProcessEvent)(BSAnimationGraphEvent& evn, BSTEventSource<BSAnimationGraphEvent>* dispatcher);

	BSEventNotifyControl HookedProcessEvent(BSAnimationGraphEvent& evn, BSTEventSource<BSAnimationGraphEvent>* src) {
		Actor* a = (Actor*)((uintptr_t)this - 0x38);
		if (a->Get3D() && a->parentCell && a->gunState == GUN_STATE::kReloading) {
			if ((evn.tag == "countDownTick") && evn.payload.length() != 0) {
				std::string boneName;
				std::string collType = SplitString(evn.payload.c_str(), "|", boneName);
				std::string velX, velY, velZ;
				boneName = SplitString(boneName, "|", velX);
				velX = SplitString(velX, "|", velY);
				velY = SplitString(velY, "|", velZ);
				for (auto& c : collType) {
					c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
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
							new (magDebris) BSTempEffectDebris(a->parentCell, fGunShellLifetime->GetFloat(), buf, a, pos, node->world.rotate, vel, NiPoint3(), 1.0f, false, true, isFP);

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
									cp.copyType = NiCloningProcess::CopyType::kCopyExact;
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
					}
					else {
						_MESSAGE("Error : Failed to find node!");
					}
				}
			}
		}
		FnProcessEvent fn = fnHash.at(*(uintptr_t*)this);
		return fn ? (this->*fn)(evn, src) : BSEventNotifyControl::kContinue;
	}

	void HookSink() {
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

void InitializePlugin() {
	p = PlayerCharacter::GetSingleton();
	((AnimationGraphEventWatcher*)((uintptr_t)p + 0x38))->HookSink();
	uintptr_t ActorVtable = REL::Relocation<uintptr_t>{ Actor::VTABLE[3] }.address();
	((AnimationGraphEventWatcher*)(&ActorVtable))->HookSink();
}

void OnF4SEMessage(F4SE::MessagingInterface::Message* msg) {
	if (msg->type == F4SE::MessagingInterface::kGameDataReady) {
		InitializePlugin();
	}
}

F4SEPluginLoad(const F4SE::LoadInterface* a_f4se) {
	F4SE::Init(a_f4se, {
		.log = true,
		.logName = "MagPoop",
	});
	taskInterface = F4SE::GetTaskInterface();
	const auto isOG = REX::FModule::IsRuntimeOG();
	const auto executableVersion = REX::FModule::GetExecutingModule().GetFileVersion();
	REX::INFO("detected Fallout 4 runtime={} f4seRuntimeVersion={} executableVersion={}",
		isOG ? "OG" : "AE", a_f4se->RuntimeVersion().string(), executableVersion.string());
	F4SE::GetMessagingInterface()->RegisterListener(OnF4SEMessage);
	return true;
}

extern "C"
{
	F4SE_EXPORT bool F4SEPlugin_Query(const F4SE::QueryInterface*, F4SE::PluginInfo* a_info)
	{
		const auto* versionData = F4SE::PluginVersionData::GetSingleton();
		if (!versionData) {
			return false;
		}
		a_info->name = versionData->GetPluginName().data();
		a_info->infoVersion = F4SE::PluginInfo::kVersion;
		a_info->version = versionData->pluginVersion;
		return true;
	}
}
