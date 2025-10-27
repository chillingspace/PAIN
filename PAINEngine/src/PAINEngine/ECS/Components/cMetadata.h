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
		 };
 
		 // Tag component for categorization
		 struct Tag {
			 std::set<std::string> tags;
		 };
 
		 // Editor-only visibility component
		 struct EditorVisible {
			 bool visible;
			 bool locked;
			 EditorVisible() : visible(true), locked(false) {}
		 };
 
		 struct Relation {
			 std::vector<entt::entity> children;
			 entt::entity parent;
		 };
 
		 // Group assignment component
		 struct Group {
			 std::string group_name;
			 Group(std::string const& name = "") : group_name(name) {}
		 };
	 }
 
 }