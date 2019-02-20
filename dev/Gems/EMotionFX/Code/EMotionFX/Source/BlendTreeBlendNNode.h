/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include "EMotionFXConfig.h"
#include "AnimGraphNode.h"


namespace EMotionFX
{
    // forward declarations
    class ActorInstance;

    class EMFX_API BlendTreeBlendNNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTreeBlendNNode, "{CBFFDE41-008D-45A1-AC2A-E9A25C8CE62A}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        enum
        {
            INPUTPORT_POSE_0    = 0,
            INPUTPORT_POSE_1    = 1,
            INPUTPORT_POSE_2    = 2,
            INPUTPORT_POSE_3    = 3,
            INPUTPORT_POSE_4    = 4,
            INPUTPORT_POSE_5    = 5,
            INPUTPORT_POSE_6    = 6,
            INPUTPORT_POSE_7    = 7,
            INPUTPORT_POSE_8    = 8,
            INPUTPORT_POSE_9    = 9,
            INPUTPORT_WEIGHT    = 10,
            OUTPUTPORT_POSE     = 0
        };

        enum
        {
            PORTID_INPUT_POSE_0 = 0,
            PORTID_INPUT_POSE_1 = 1,
            PORTID_INPUT_POSE_2 = 2,
            PORTID_INPUT_POSE_3 = 3,
            PORTID_INPUT_POSE_4 = 4,
            PORTID_INPUT_POSE_5 = 5,
            PORTID_INPUT_POSE_6 = 6,
            PORTID_INPUT_POSE_7 = 7,
            PORTID_INPUT_POSE_8 = 8,
            PORTID_INPUT_POSE_9 = 9,
            PORTID_INPUT_WEIGHT = 10,

            PORTID_OUTPUT_POSE  = 0
        };

        class EMFX_API UniqueData
            : public AnimGraphNodeData
        {
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE
        public:
            AZ_CLASS_ALLOCATOR_DECL

            UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance, uint32 indexA, uint32 indexB)
                : AnimGraphNodeData(node, animGraphInstance)       { mIndexA = indexA; mIndexB = indexB; }

        public:
            uint32  mIndexA;
            uint32  mIndexB;
        };

        BlendTreeBlendNNode();
        ~BlendTreeBlendNNode();

        bool InitAfterLoading(AnimGraph* animGraph) override;

        void OnUpdateUniqueData(AnimGraphInstance* animGraphInstance) override;
        bool GetHasOutputPose() const override                  { return true; }
        bool GetSupportsDisable() const override                { return true; }
        bool GetSupportsVisualization() const override          { return true; }
        uint32 GetVisualColor() const override                  { return MCore::RGBA(159, 81, 255); }

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override     { return GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue(); }

        void FindBlendNodes(AnimGraphInstance* animGraphInstance, AnimGraphNode** outNodeA, AnimGraphNode** outNodeB, uint32* outIndexA, uint32* outIndexB, float* outWeight) const;

        static void Reflect(AZ::ReflectContext* context);
        void SetSyncMode(ESyncMode syncMode);
        void SetEventMode(EEventMode eventMode);

    private:
        void SyncMotions(AnimGraphInstance* animGraphInstance, AnimGraphNode* nodeA, AnimGraphNode* nodeB, uint32 poseIndexA, uint32 poseIndexB, float blendWeight, ESyncMode syncMode);
        void Output(AnimGraphInstance* animGraphInstance) override;
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;

        ESyncMode       m_syncMode;
        EEventMode      m_eventMode;
    };
}   // namespace EMotionFX
