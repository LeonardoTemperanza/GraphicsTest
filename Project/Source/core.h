
#pragma once

#include "os/os_generic.h"
#include "renderer/renderer_generic.h"

struct Model;

enum EntityFlags
{
    EntityFlags_Static      = 1 << 0,
    EntityFlags_Destroyed   = 1 << 1,
    EntityFlags_Transparent = 1 << 2
};

enum EntityKind
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

struct Entity
{
    // These are local with respect to the mounted entity
    Vec3 pos;
    Quat rot;
    Vec3 scale;
    
    u16 flags;
    u32 gen;
    
    // We have the option to get the derived
    // entity, though it's a bit harder than the other way around
    u8 derivedKind;
    u16 derivedId;  // Index in the corresponding array
    
    Model* model;  // Can be nullptr for entities with no model
    
    EntityKey mount;
    u16 mountBone;
};

struct Camera
{
    Entity* base;
    
    float horizontalFOV;  // Degrees
    float nearClip;
    float farClip;
};

struct Player
{
    Entity* base;
    
    Vec3 speed;
    bool grounded;
};

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
    // ensures pointer stability
    
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
};

Entities* InitEntities();
void FreeEntities(Entities* entities);
void MainUpdate(Entities* entities, float deltaTime, Arena* permArena, Arena* frameArena);
void UpdateEntities(Entities* entities, float deltaTime);
void UpdateUI();
void RenderEntities(Entities* entities, float deltaTime);

// Entity manipulation.
// All functions returning entity pointers can return null
template<typename t>
EntityKind GetEntityKindFromType();
template<typename t>
Array<t>* GetArrayFromType();

EntityKey NullKey();
bool IsKeyNull(EntityKey key);

Entity* GetEntity(EntityKey key);  // Will return null if the entity does not exist
template<typename t>
t* GetDerived(EntityKey key);
template<typename t>
t* GetDerived(DerivedKey<t> key);
EntityKey GetKey(Entity* entity);
Entity* GetMount(Entity* entity);
void MountEntity(Entity* entity, Entity* mountTo);
Mat4 ComputeWorldTransform(Entity* entity);

Entity* NewEntity();
template<typename t>
t* NewEntity();

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

// Gameplay code
void UpdateMainCamera(Camera* camera, float deltaTime);
void UpdatePlayer(Player* player, float deltaTime);
