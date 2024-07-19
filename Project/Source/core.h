
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
    Entity_Count
};

struct EntityKey
{
    u16 id;
    u16 gen;
};

#define NullId 0
struct Entity
{
    // These are local with respect to the mounted entity
    Vec3 pos;
    Quat rot;
    Vec3 scale;
    
    u16 flags;
    u16 gen;
    
    // We have the option to get the derived
    // entity, though it's a bit harder
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

// NOTE: Arenas are used all entity arrays here, which
// ensures pointer stability

template<typename t>
struct DerivedArray
{
    Array<t>   all;
    Array<u16> free;
};

struct BaseArray
{
    Array<Entity> all;
    Array<u32> free;
};

template<typename t>
void Free(DerivedArray<t>* array);
void Free(BaseArray* array);

struct Entities
{
    EntityKey mainCamera;  // Used by the renderer
    
    // NOTE: Arenas are used all entity arrays here, which
    // ensures pointer stability
    
    // Info all entities have in common
    BaseArray bases;
    
    // Specific entity types
    DerivedArray<Camera> cameras;
    DerivedArray<Player> players;
};

// Main simulation
Entities InitEntities();
void FreeEntities(Entities* entities);
void MainUpdate(Entities* entities, float deltaTime, Arena* permArena, Arena* frameArena);
void UpdateEntities(Entities* entities, float deltaTime);
void UpdateUI();
void RenderEntities(Entities* entities, float deltaTime);

// Entity manipulation
u32 NewEntity(Entities* entities);
template<typename t>
u16 NewDerived(Entities* entities, DerivedArray<t>* derived);

// Does not remove anything and can safely be used
// while iterating over the arrays.
// Can be used with any BaseArray/DerivedArray
template<typename t>
void DestroyEntity(t entityArray, u32 id);
u32  LookupEntity(Entities* entities, EntityKey key);
u32  LookupEntity(Entities* entities, EntityKey key, EntityKind kind);
void MountEntity(Entities* entities, u32 mounted, u32 mountTo);
Mat4 ComputeWorldTransform(Entities* entities, u32 id);
void ComputeWorldTransform(Entities* entities, u32 id, Vec3* pos, Quat* rot, Vec3* scale);

// TODO: Entity iteration


// Gameplay code (update functions)
void UpdateMainCamera(Camera* camera, float deltaTime);
void UpdatePlayer(Player* player, float deltaTime);