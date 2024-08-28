
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
    
    // We have the option to get the derived
    // entity, though it's a bit harder than the other way around
    EntityKind derivedKind;
    u16 derivedId;  // Index in the corresponding array
    
    AssetKey model;  // Can be null
    
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
};

introspect()
struct PointLight
{
    Entity* base;
    
    float intensity;
    Vec3 offset;
};

struct Entities
{
    EntityKey mainCamera;  // Used by the renderer
    
    // NOTE: Arenas are used for all entity arrays here, which
    // ensures pointer stability. Maybe to better convey this
    // I should add a "StableArray" type?
    
    // Info all entities have in common
    Array<Entity> bases;
    Array<u32> freeBases;
    
    // Specific entity types
    // These don't need to be stable
    // as keys refer to the base entity
    // not the derived one. So no need
    // for a free list/array
    Array<Camera> cameras;
    Array<Player> players;
    Array<PointLight> pointLights;
    
    // Components here maybe?
    
    // Per frame data
    // NOTE: This is editor only for now. It's pretty
    // expensive so it shouldn't be done every frame in release
    Slice<Slice<Entity*>> liveChildrenPerEntity;
};

struct Editor;

Entities* InitEntities();
void FreeEntities(Entities* entities);
void MainUpdate(Entities* entities, Editor* ui, float deltaTime, Arena* permArena, Arena* frameArena);
void UpdateEntities(Entities* entities, float deltaTime);
void RenderEntities(Entities* entities, Editor* editor, bool inEditor, float deltaTime);

// Entity manipulation.
// All functions returning entity pointers can return null
template<typename t>
EntityKind GetEntityKindFromType();
template<typename t>
Array<t>* GetArrayFromType();

bool operator ==(EntityKey k1, EntityKey k2);
bool operator !=(EntityKey k1, EntityKey k2);
EntityKey NullKey();
bool IsKeyNull(EntityKey key);

Entity* GetEntity(EntityKey key);  // Will return null if the entity does not exist
Entity* GetEntity(u32 id);
template<typename t>
t* GetDerived(EntityKey key);
template<typename t>
t* GetDerived(Entity* entity);
template<typename t>
t* GetDerived(DerivedKey<t> key);
void* GetDerivedAddr(u32 id);
EntityKey GetKey(Entity* entity);
u32 GetId(Entity* entity);
template<typename t>
DerivedKey<t> GetDerivedKey(t* derived);
Entity* GetMount(Entity* entity);
// Pass null to mountTo to unmount from any entity
void MountEntity(Entity* entity, Entity* mountTo);
Mat4 ComputeWorldTransform(Entity* entity);
Mat4 ConvertToLocalTransform(Entity* entity, Mat4 world);

Entity* NewEntity();
template<typename t>
t* NewEntity();

int GetNumEntities();
Slice<Slice<Entity*>> GetLiveChildrenPerEntity();

void DestroyEntity(Entity* entity);
void CommitDestroy(Entities* entities);

// Entity iteration
Entity* FirstLive();
Entity* NextLive(Entity* current);
template<typename t>
t* FirstDerivedLive();
template<typename t>
t* NextDerivedLive(t* current);

#define for_live_entities(name) for(Entity* name = FirstLive(); name; name = NextLive(name))
#define for_live_derived(name, type) for(type* name = FirstDerivedLive<type>(); name; name = NextDerivedLive<type>(name))

// Utilities
Camera* GetMainCamera();
// Slices only contain direct children
Slice<Slice<Entity*>> ComputeAllLiveChildrenForEachEntity(Arena* dst);
// Checks if an entity is a direct or indirect child of another
bool IsChild(Entity* suspectedChild, Entity* entity, Slice<Slice<Entity*>> childrenPerEntity);

// Gameplay code
void UpdatePlayer(Player* player, float deltaTime);
