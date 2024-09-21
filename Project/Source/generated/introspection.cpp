
/*
 * This file was generated by 'metaprogram.cpp'. It contains information about
 * various datastructures to be able to use introspection
*/

#include "generated/introspection.h"
#include "base.h"
// Including all project headers to pick up struct declarations
#include "asset_system.h"
#include "base.h"
#include "core.h"
#include "editor.h"
#include "input.h"
#include "gameplay.h"

MemberDefinition _membersOfAssetKey[] =
{
{ { Meta_Unknown }, offsetof(AssetKey, path), sizeof(((AssetKey*)0)->path), StrLit("AssetKey"), "AssetKey", StrLit("Path"), "Path", 0, true},
{ { Meta_Int }, offsetof(AssetKey, a), sizeof(((AssetKey*)0)->a), StrLit("AssetKey"), "AssetKey", StrLit("A"), "A", 0, true},
};

MetaStruct metaAssetKey =
{ {.ptr=_membersOfAssetKey, .len=ArrayCount(_membersOfAssetKey)}, StrLit("AssetKey"), "AssetKey" };

MemberDefinition _membersOfEntity[] =
{
{ { Meta_Vec3 }, offsetof(Entity, pos), sizeof(((Entity*)0)->pos), StrLit("Entity"), "Entity", StrLit("Position"), "Position", 0, true},
{ { Meta_Quat }, offsetof(Entity, rot), sizeof(((Entity*)0)->rot), StrLit("Entity"), "Entity", StrLit("Rotation"), "Rotation", 0, true},
{ { Meta_Vec3 }, offsetof(Entity, scale), sizeof(((Entity*)0)->scale), StrLit("Entity"), "Entity", StrLit("Scale"), "Scale", 0, true},
{ { Meta_Unknown }, offsetof(Entity, flags), sizeof(((Entity*)0)->flags), StrLit("Entity"), "Entity", StrLit("Flags"), "Flags", 0, true},
{ { Meta_Unknown }, offsetof(Entity, gen), sizeof(((Entity*)0)->gen), StrLit("Entity"), "Entity", StrLit("Gen"), "Gen", 0, true},
{ { Meta_Unknown }, offsetof(Entity, mesh), sizeof(((Entity*)0)->mesh), StrLit("Entity"), "Entity", StrLit("Mesh"), "Mesh", 0, true},
{ { Meta_Unknown }, offsetof(Entity, material), sizeof(((Entity*)0)->material), StrLit("Entity"), "Entity", StrLit("Material"), "Material", 0, true},
{ { Meta_Unknown }, offsetof(Entity, derivedKind), sizeof(((Entity*)0)->derivedKind), StrLit("Entity"), "Entity", StrLit("Derived Kind"), "Derived Kind", 0, true},
{ { Meta_Unknown }, offsetof(Entity, derivedId), sizeof(((Entity*)0)->derivedId), StrLit("Entity"), "Entity", StrLit("Derived Id"), "Derived Id", 0, true},
{ { Meta_Unknown }, offsetof(Entity, mount), sizeof(((Entity*)0)->mount), StrLit("Entity"), "Entity", StrLit("Mount"), "Mount", 0, true},
{ { Meta_Unknown }, offsetof(Entity, mountBone), sizeof(((Entity*)0)->mountBone), StrLit("Entity"), "Entity", StrLit("Mount Bone"), "Mount Bone", 0, true},
};

MetaStruct metaEntity =
{ {.ptr=_membersOfEntity, .len=ArrayCount(_membersOfEntity)}, StrLit("Entity"), "Entity" };

MemberDefinition _membersOfCamera[] =
{
{ { Meta_Unknown }, offsetof(Camera, base), sizeof(((Camera*)0)->base), StrLit("Camera"), "Camera", StrLit("Base"), "Base", 0, true},
{ { Meta_Unknown }, offsetof(Camera, params), sizeof(((Camera*)0)->params), StrLit("Camera"), "Camera", StrLit("Params"), "Params", 0, true},
};

MetaStruct metaCamera =
{ {.ptr=_membersOfCamera, .len=ArrayCount(_membersOfCamera)}, StrLit("Camera"), "Camera" };

MemberDefinition _membersOfPlayer[] =
{
{ { Meta_Unknown }, offsetof(Player, base), sizeof(((Player*)0)->base), StrLit("Player"), "Player", StrLit("Base"), "Base", 0, true},
{ { Meta_Vec3 }, offsetof(Player, speed), sizeof(((Player*)0)->speed), StrLit("Player"), "Player", StrLit("Speed"), "Speed", 0, false},
{ { Meta_Bool }, offsetof(Player, grounded), sizeof(((Player*)0)->grounded), StrLit("Player"), "Player", StrLit("Grounded"), "Grounded", 0, false},
{ { Meta_Float }, offsetof(Player, gravity), sizeof(((Player*)0)->gravity), StrLit("Player"), "Player", StrLit("Gravity"), "Gravity", 0, true},
{ { Meta_Float }, offsetof(Player, jumpVel), sizeof(((Player*)0)->jumpVel), StrLit("Player"), "Player", StrLit("Jump Vel"), "Jump Vel", 0, true},
{ { Meta_Float }, offsetof(Player, moveSpeed), sizeof(((Player*)0)->moveSpeed), StrLit("Player"), "Player", StrLit("Move Speed"), "Move Speed", 0, true},
{ { Meta_Float }, offsetof(Player, groundAccel), sizeof(((Player*)0)->groundAccel), StrLit("Player"), "Player", StrLit("Ground Accel"), "Ground Accel", 0, true},
{ { Meta_Float }, offsetof(Player, newGravity), sizeof(((Player*)0)->newGravity), StrLit("Player"), "Player", StrLit("New Gravity"), "New Gravity", 2, true},
};

MetaStruct metaPlayer =
{ {.ptr=_membersOfPlayer, .len=ArrayCount(_membersOfPlayer)}, StrLit("Player"), "Player" };

MemberDefinition _membersOfPointLight[] =
{
{ { Meta_Unknown }, offsetof(PointLight, base), sizeof(((PointLight*)0)->base), StrLit("PointLight"), "PointLight", StrLit("Base"), "Base", 0, true},
{ { Meta_Float }, offsetof(PointLight, intensity), sizeof(((PointLight*)0)->intensity), StrLit("PointLight"), "PointLight", StrLit("Intensity"), "Intensity", 0, true},
{ { Meta_Vec3 }, offsetof(PointLight, offset), sizeof(((PointLight*)0)->offset), StrLit("PointLight"), "PointLight", StrLit("Offset"), "Offset", 0, true},
};

MetaStruct metaPointLight =
{ {.ptr=_membersOfPointLight, .len=ArrayCount(_membersOfPointLight)}, StrLit("PointLight"), "PointLight" };

