#pragma once

#define PAIN_API

// Components
#include "ECS/Components/cPhysics.h"
#include "ECS/Components/cTransform.h"
#include "ECS/Components/cMeshRenderer.h"
#include "ECS/Components/cAnimation.h"
#include "ECS/Components/cMetadata.h"
#include "ECS/Components/cLight.h"
#include "ECS/Components/cScript.h"
#include "ECS/Components/cAudioSource.h"
#include "ECS/Components/cAI.h"
#include "ECS/Components/cCamera.h"
#include "ECS/Components/cBoundingVolume.h"
#include "ECS/Components/cEntity.h"
#include "ECS/Components/cPrefab.h"
#include "ECS/Components/cUIComps.h"

const std::string ENGINE_NAME = "Pain Engine";
const unsigned int DEF_ENGINE_HEIGHT = 900;
const unsigned int DEF_ENGINE_WIDTH = 1600;
