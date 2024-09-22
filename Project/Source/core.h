
#pragma once

#include "os/os_generic.h"
#include "renderer/renderer_generic.h"
#include "asset_system.h"

enum EntityFlags
{
    EntityFlags_Static           = 1 << 0,
    EntityFlags_Destroyed        = 1 << 1,
    EntityFlags_Visible          = 1 << 3,
    EntityFlags_Active           = 1 << 4,
    EntityFlags_IgnoreMountPos   = 1 << 5,
    EntityFlags_IgnoreMountRot   = 1 << 6,
    EntityFlags_IgnoreMountScale = 1 << 7
};

enum EntityKind: u8
{
    Entity_None = 0,
    Entity_Camera,
    Entity_Player,
    Entity_PointLight,
    
    Entity_Count
};

struct EntityKey
{
    u32 id;
    u32 gen;
};

template<typename t>
struct DerivedKey
{
    u16 id;
    u32 gen;
};

introspect()
struct Entity
{
    // These are local with respect to the mounted entity
    nice_name("Position");
    Vec3 pos;
    nice_name("Rotation");
    Quat rot;
    nice_name("Scale");
    Vec3 scale;
    
    u16 flags;
    u32 gen;
    
    AssetKey mesh;
    AssetKey material;
    
    // We have the option to get the derived
    // entity, though it's a bit harder than the other way around
    EntityKind derivedKind;
    u16 derivedId;  // Index in the corresponding array
    
    EntityKey mount;
    u16 mountBone;
};

introspect()
struct Camera
{
    Entity* base;
    
    CameraParams params;
};

introspect()
struct Player
{
    Entity* base;
    
    editor_hide;
    Vec3 speed;
    editor_hide;
    bool grounded;
    
    // Movement constants
    float gravity;
    float jumpVel;
    float moveSpeed;
    float groundAccel;
    
    member_version(2);
    float newGravity;
};

introspect()
struct PointLight
{
    Entity* base;
    
    float intensity;
    Vec3 offset;
};

struct EntityManager
{
    EntityKey mainCamera;  // Used by the renderer
    
    // NOTE: Arenas are used for all entity arrays here, which
    // ensures pointer stability. This is so we can simply put
    // a pointer the base entity in the derived entity, instead
    // of using an index and call a function every time which is
    // a lot less ergonomic
    
    // Info all entities have in common
    Array<Entity> bases;
    Array<u32> freeBases;
    
    Array<Camera> cameras;
    Array<Player> players;
    Array<PointLight> pointLights;
    
    // Per frame data
    
    // NOTE: This is editor only for now. It's a bit
    // expensive so it shouldn't be computed every frame in release
    Slice<Slice<Entity*>> liveChildrenPerEntity;
    
    Array<PointLight> framePointLights;
};

struct Editor;

EntityManager InitEntityManager();
void FreeEntities(EntityManager* man);
void MainUpdate(EntityManager* man, Editor* ui, float deltaTime, Arena* frameArena);
void MainRender(EntityManager* man, Editor* ui, float deltaTime, Arena* frameArena);
void UpdateEntities(EntityManager* man, float deltaTime);

// Entity manipulation.
// All functions returning entity pointers can return null
template<typename t>
EntityKind GetEntityKindFromType(EntityManager* man);
template<typename t>
Array<t>* GetArrayFromType(EntityManager* man);

bool operator ==(EntityKey k1, EntityKey k2);
bool operator !=(EntityKey k1, EntityKey k2);
EntityKey NullKey();
bool IsKeyNull(EntityKey key);

Entity* GetEntity(EntityManager* man, EntityKey key);  // Will return null if the entity does not exist
Entity* GetEntity(EntityManager* man, u32 id);
template<typename t>
t* GetDerived(EntityManager* man, EntityKey key);
template<typename t>
t* GetDerived(EntityManager* man, Entity* entity);
template<typename t>
t* GetDerived(EntityManager* man, DerivedKey<t> key);
void* GetDerivedAddr(EntityManager* man, u32 id);
EntityKey GetKey(EntityManager* man, Entity* entity);
u32 GetId(EntityManager* man, Entity* entity);
template<typename t>
DerivedKey<t> GetDerivedKey(EntityManager* man, t* derived);
Entity* GetMount(EntityManager* man, Entity* entity);
// Pass null to mountTo to unmount from any entity
void MountEntity(EntityManager* man, Entity* entity, Entity* mountTo);
Mat4 ComputeWorldTransform(EntityManager* man, Entity* entity);
Mat4 ConvertToLocalTransform(EntityManager* man, Entity* entity, Mat4 world);

Entity* NewEntity(EntityManager* man);
template<typename t>
t* NewEntity(EntityManager* man);

void DestroyEntity(Entity* entity);
void CommitDestroy(EntityManager* entities);

// Entity iteration
Entity* FirstLive(EntityManager* man);
Entity* NextLive(EntityManager* man, Entity* current);
template<typename t>
t* FirstDerivedLive(EntityManager* man);
template<typename t>
t* NextDerivedLive(EntityManager* man, t* current);

#define for_live_entities(manager, name) for(Entity* name = FirstLive(manager); name; name = NextLive(manager, name))
#define for_live_derived(manager, name, type) for(type* name = FirstDerivedLive<type>(manager); name; name = NextDerivedLive<type>(manager, name))

// Utilities
// Slices only contain direct children
Slice<Slice<Entity*>> ComputeAllLiveChildrenForEachEntity(EntityManager* man, Arena* dst);
// Checks if an entity is a direct or indirect child of another
bool IsChild(EntityManager* man, Entity* suspectedChild, Entity* entity);

// Gameplay code
void UpdatePlayer(Player* player, float deltaTime);
