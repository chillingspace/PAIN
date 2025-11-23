/*****************************************************************//**
 * \file   cMetaData.h
 * \brief  All physics data components
 *
 * \author Bryan Lim, 2301214, bryanlicheng.l@digipen.edu (100%)
 * \co-author
 * \date   September 2025
 * All content 2024 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#pragma once

#include "pch.h"

 namespace PAIN {
 
	 namespace MetaData {
		 /******************************************************************************************
		 * Note: When creating components, try to stack them properly to properly optimise memory
		 * (Place largest type var (Double) first, then followed by smallest.
		 *****************************************************************************************/
 
		 // Core identity component (always present)
		 struct EntityName {
			 std::string name;
			 EntityName(std::string const& n = "entity_") : name(n) {}

			 //Serialization flag
			 static constexpr bool ShouldSerialize = true;
		 };
 
		 // Tag component for categorization
		 struct Tag {
			 std::set<std::string> tags;

			 //Serialization flag
			 static constexpr bool ShouldSerialize = true;
		 };
 
		 // Editor-only visibility component
		 struct EditorVisible {
			 bool visible;
			 bool locked;
			 EditorVisible() : visible(true), locked(false) {}

			 //Serialization flag
			 static constexpr bool ShouldSerialize = true;
		 };
 
		 struct Relation {
			 std::vector<entt::entity> children;
			 entt::entity parent;

			 //Serialization flag
			 static constexpr bool ShouldSerialize = true;
		 };
 
		 // Group assignment component
		 struct Group {
			 std::string group_name;
			 Group(std::string const& name = "") : group_name(name) {}

			 //Serialization flag
			 static constexpr bool ShouldSerialize = true;
		 };
	 }
 
 }

REFL_TYPE(PAIN::MetaData::EntityName)
REFL_FIELD(name)
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::MetaData::EntityName>);

REFL_TYPE(PAIN::MetaData::Tag)
REFL_FIELD(tags)
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::MetaData::Tag>);

REFL_TYPE(PAIN::MetaData::EditorVisible)
REFL_FIELD(visible)
REFL_FIELD(locked)
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::MetaData::EditorVisible>);

REFL_TYPE(PAIN::MetaData::Relation)
REFL_FIELD(children)
REFL_FIELD(parent)
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::MetaData::Relation>);

REFL_TYPE(PAIN::MetaData::Group)
REFL_FIELD(group_name)
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::MetaData::Group>);