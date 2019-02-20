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

#include "MorphTargetsWindowPlugin.h"
#include <AzCore/Casting/numeric_cast.h>
#include <QPushButton>
#include <QLabel>
#include "../../../../EMStudioSDK/Source/EMStudioCore.h"
#include <MysticQt/Source/ButtonGroup.h>
#include <MCore/Source/LogManager.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/MorphSetup.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"


namespace EMStudio
{
    // constructor
    MorphTargetsWindowPlugin::MorphTargetsWindowPlugin()
        : EMStudio::DockWidgetPlugin()
    {
        mDialogStack                    = nullptr;
        mSelectCallback                 = nullptr;
        mUnselectCallback               = nullptr;
        mClearSelectionCallback         = nullptr;
        mCurrentActorInstance           = nullptr;
        mAdjustMorphTargetCallback      = nullptr;
        mAdjustActorInstanceCallback    = nullptr;
    }


    // destructor
    MorphTargetsWindowPlugin::~MorphTargetsWindowPlugin()
    {
        // unregister the command callbacks and get rid of the memory
        GetCommandManager()->RemoveCommandCallback(mSelectCallback, false);
        GetCommandManager()->RemoveCommandCallback(mUnselectCallback, false);
        GetCommandManager()->RemoveCommandCallback(mClearSelectionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAdjustMorphTargetCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAdjustActorInstanceCallback, false);
        delete mSelectCallback;
        delete mUnselectCallback;
        delete mClearSelectionCallback;
        delete mAdjustMorphTargetCallback;
        delete mAdjustActorInstanceCallback;

        Clear();

        // delete the dialog stack
        delete mDialogStack;
    }


    // clone the log window
    EMStudioPlugin* MorphTargetsWindowPlugin::Clone()
    {
        MorphTargetsWindowPlugin* newPlugin = new MorphTargetsWindowPlugin();
        return newPlugin;
    }


    // init after the parent dock window has been created
    bool MorphTargetsWindowPlugin::Init()
    {
        // create the static text layout
        mStaticTextWidget = new QWidget();
        mStaticTextLayout = new QVBoxLayout();
        mStaticTextWidget->setLayout(mStaticTextLayout);
        QLabel* label = new QLabel("No morph targets to show.");
        mStaticTextLayout->addWidget(label);
        mStaticTextLayout->setAlignment(label, Qt::AlignCenter);

        // create the dialog stack
        assert(mDialogStack == nullptr);
        mDialogStack = new MysticQt::DialogStack();
        mDock->setMinimumWidth(300);
        mDock->setMinimumHeight(100);
        mDock->SetContents(mStaticTextWidget);

        // create and register the command callbacks only (only execute this code once for all plugins)
        mSelectCallback             = new CommandSelectCallback(false);
        mUnselectCallback           = new CommandUnselectCallback(false);
        mClearSelectionCallback     = new CommandClearSelectionCallback(false);
        mAdjustMorphTargetCallback  = new CommandAdjustMorphTargetCallback(false);
        mAdjustActorInstanceCallback = new CommandAdjustActorInstanceCallback(false);

        GetCommandManager()->RegisterCommandCallback("Select", mSelectCallback);
        GetCommandManager()->RegisterCommandCallback("Unselect", mUnselectCallback);
        GetCommandManager()->RegisterCommandCallback("ClearSelection", mClearSelectionCallback);
        GetCommandManager()->RegisterCommandCallback("AdjustMorphTarget", mAdjustMorphTargetCallback);
        GetCommandManager()->RegisterCommandCallback("AdjustActorInstance", mAdjustActorInstanceCallback);

        // reinit the dialog
        ReInit();

        // connect the window activation signal to refresh if reactivated
        connect(mDock, SIGNAL(visibilityChanged(bool)), this, SLOT(WindowReInit(bool)));

        // done
        return true;
    }


    // clear the morph target window
    void MorphTargetsWindowPlugin::Clear()
    {
        mDock->SetContents(mStaticTextWidget);

        // clear the dialog stack
        mDialogStack->Clear();

        mMorphTargetGroups.clear();
    }


    // reinit the morph target dialog, e.g. if selection changes
    void MorphTargetsWindowPlugin::ReInit(bool forceReInit)
    {
        // get the selected actorinstance
        const CommandSystem::SelectionList& selection       = GetCommandManager()->GetCurrentSelection();
        EMotionFX::ActorInstance*           actorInstance   = selection.GetSingleActorInstance();

        // show hint if no/multiple actor instances is/are selected
        if (actorInstance == nullptr)
        {
            // set the dock contents
            mDock->SetContents(mStaticTextWidget);

            // clear dialog and reset the current actor instance as we cleared the window
            if (mCurrentActorInstance)
            {
                Clear();
                mCurrentActorInstance = nullptr;
            }

            // done
            return;
        }

        // get our selected actor instance and the corresponding actor
        EMotionFX::Actor* actor = actorInstance->GetActor();

        // only reinit the morph targets if actorinstance changed
        if (mCurrentActorInstance != actorInstance || forceReInit)
        {
            // set the current actor instance in any case
            mCurrentActorInstance = actorInstance;

            // arrays for the default morph targets and the phonemes
            AZStd::vector<EMotionFX::MorphTarget*>                       phonemes;
            AZStd::vector<EMotionFX::MorphTarget*>                       defaultMorphTargets;
            AZStd::vector<EMotionFX::MorphSetupInstance::MorphTarget*>   phonemeInstances;
            AZStd::vector<EMotionFX::MorphSetupInstance::MorphTarget*>   defaultMorphTargetInstances;

            // get the morph target setup
            EMotionFX::MorphSetup* morphSetup = actor->GetMorphSetup(actorInstance->GetLODLevel());
            if (morphSetup == nullptr)
            {
                mDock->SetContents(mStaticTextWidget);
                return;
            }

            // get the corresponding morph setup instance from the actor instance, this holds the weight values
            EMotionFX::MorphSetupInstance* morphSetupInstance = actorInstance->GetMorphSetupInstance();
            if (morphSetupInstance == nullptr)
            {
                mDock->SetContents(mStaticTextWidget);
                return;
            }

            // get the number of morph targets
            const size_t numMorphTargets = aznumeric_caster(morphSetup->GetNumMorphTargets());

            // preinit array size
            phonemes.reserve(numMorphTargets);
            phonemeInstances.reserve(numMorphTargets);
            defaultMorphTargets.reserve(numMorphTargets);
            defaultMorphTargetInstances.reserve(numMorphTargets);

            // iterate trough all morph targets
            for (size_t i = 0; i < numMorphTargets; ++i)
            {
                // get the current morph target
                EMotionFX::MorphTarget* morphTarget = morphSetup->GetMorphTarget(aznumeric_caster(i));

                // get the corresponding morph target instance
                // this contains the weight value and some other settings that are unique for the morph target inside this actor instance
                EMotionFX::MorphSetupInstance::MorphTarget* morphTargetInstance = morphSetupInstance->FindMorphTargetByID(morphTarget->GetID());
                if (morphTargetInstance == nullptr)
                {
                    AZ_Error("EMotionFX", false, "No corresponding morph target instance found for morph target '%s'.", morphTarget->GetName());
                    continue;
                }

                // don't add phoneme morph targets (they are used for lipsync)
                if (morphTarget->GetIsPhoneme())
                {
                    phonemes.push_back(morphTarget);
                    phonemeInstances.push_back(morphTargetInstance);
                }
                else
                {
                    defaultMorphTargets.push_back(morphTarget);
                    defaultMorphTargetInstances.push_back(morphTargetInstance);
                }
            }

            // clear the window in case we have already had any widgets in it
            Clear();

            // create group for the default morph targets
            if (!defaultMorphTargets.empty())
            {
                CreateGroup("Default", defaultMorphTargets, defaultMorphTargetInstances);
            }

            // create group for the phonemes
            if (!phonemes.empty())
            {
                CreateGroup("Phonemes", phonemes, phonemeInstances);
            }

            // create static text if no morph targets are available
            if (defaultMorphTargets.empty() && phonemes.empty())
            {
                mDock->SetContents(mStaticTextWidget);
            }
            else
            {
                mDock->SetContents(mDialogStack);
            }

            // adjust the slider values to the correct weights of the selected actor instance
            UpdateInterface();
        }
    }


    void MorphTargetsWindowPlugin::CreateGroup(const char* name, const AZStd::vector<EMotionFX::MorphTarget*>& morphTargets, const AZStd::vector<EMotionFX::MorphSetupInstance::MorphTarget*>& morphTargetInstances)
    {
        if (morphTargets.empty() || morphTargetInstances.empty())
        {
            return;
        }

        MorphTargetGroupWidget* morphTargetGroup = new MorphTargetGroupWidget(name, mCurrentActorInstance, morphTargets, morphTargetInstances, mDialogStack);
        mMorphTargetGroups.push_back(morphTargetGroup);

        mDialogStack->Add(morphTargetGroup, name);
    }


    // reinit the window when it gets activated
    void MorphTargetsWindowPlugin::WindowReInit(bool visible)
    {
        if (visible)
        {
            ReInit(true);
        }
    }


    // update the interface
    void MorphTargetsWindowPlugin::UpdateInterface()
    {
        for (MorphTargetGroupWidget* group : mMorphTargetGroups)
        {
            group->UpdateInterface();
        }
    }


    // update the morph target
    void MorphTargetsWindowPlugin::UpdateMorphTarget(const char* name)
    {
        for (MorphTargetGroupWidget* group : mMorphTargetGroups)
        {
            group->UpdateMorphTarget(name);
        }
    }


    //-----------------------------------------------------------------------------------------
    // Command callbacks
    //-----------------------------------------------------------------------------------------


    bool ReInitMorphTargetsWindowPlugin()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MorphTargetsWindowPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return false;
        }

        MorphTargetsWindowPlugin* morphTargetsWindow = (MorphTargetsWindowPlugin*)plugin;

        // is the plugin visible? only update it if it is visible
        if (morphTargetsWindow->GetDockWidget()->visibleRegion().isEmpty() == false)
        {
            morphTargetsWindow->ReInit(true);
        }

        return true;
    }


    bool UpdateMorphTargetsWindowPluginInterface(const char* name)
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MorphTargetsWindowPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return false;
        }

        MorphTargetsWindowPlugin* morphTargetsWindow = (MorphTargetsWindowPlugin*)plugin;

        // is the plugin visible? only update it if it is visible
        if (morphTargetsWindow->GetDockWidget()->visibleRegion().isEmpty() == false)
        {
            morphTargetsWindow->UpdateMorphTarget(name);
        }

        return true;
    }


    bool MorphTargetsWindowPlugin::CommandSelectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return ReInitMorphTargetsWindowPlugin();
    }
    bool MorphTargetsWindowPlugin::CommandSelectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return ReInitMorphTargetsWindowPlugin();
    }
    bool MorphTargetsWindowPlugin::CommandUnselectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return ReInitMorphTargetsWindowPlugin();
    }
    bool MorphTargetsWindowPlugin::CommandUnselectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return ReInitMorphTargetsWindowPlugin();
    }
    bool MorphTargetsWindowPlugin::CommandClearSelectionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)       { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitMorphTargetsWindowPlugin(); }
    bool MorphTargetsWindowPlugin::CommandClearSelectionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)          { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitMorphTargetsWindowPlugin(); }


    bool MorphTargetsWindowPlugin::CommandAdjustMorphTargetCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZStd::string name;
        commandLine.GetValue("name", command, &name);
        return UpdateMorphTargetsWindowPluginInterface(name.c_str());
    }


    bool MorphTargetsWindowPlugin::CommandAdjustMorphTargetCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZStd::string name;
        commandLine.GetValue("name", command, &name);
        return UpdateMorphTargetsWindowPluginInterface(name.c_str());
    }


    bool MorphTargetsWindowPlugin::CommandAdjustActorInstanceCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (commandLine.CheckIfHasParameter("lodLevel"))
        {
            return ReInitMorphTargetsWindowPlugin();
        }

        return true;
    }


    bool MorphTargetsWindowPlugin::CommandAdjustActorInstanceCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (commandLine.CheckIfHasParameter("lodLevel"))
        {
            return ReInitMorphTargetsWindowPlugin();
        }

        return true;
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/MorphTargetsWindow/MorphTargetsWindowPlugin.moc>