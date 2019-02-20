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

#include "Vector3GizmoParameterEditor.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Rendering/Common/TranslateManipulator.h>
#include <EMotionFX/Source/Parameter/Vector3Parameter.h>
#include <EMotionStudio/EMStudioSDK/Source/Allocators.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <MCore/Source/AttributeVector3.h>

namespace EMStudio
{
    AZ_CLASS_ALLOCATOR_IMPL(Vector3GizmoParameterEditor, EMStudio::UIAllocator, 0)

    Vector3GizmoParameterEditor::Vector3GizmoParameterEditor(EMotionFX::AnimGraph* animGraph, const EMotionFX::ValueParameter* valueParameter, const AZStd::vector<MCore::Attribute*>& attributes)
        : ValueParameterEditor(animGraph, valueParameter, attributes)
        , m_currentValue(0.0f, 0.0f, 0.0f)
        , m_gizmoButton(nullptr)
        , m_transformationGizmo(nullptr)
    {
        UpdateValue();
    }

    Vector3GizmoParameterEditor::~Vector3GizmoParameterEditor()
    {
        if (m_transformationGizmo)
        {
            GetManager()->RemoveTransformationManipulator(m_transformationGizmo);
            delete m_transformationGizmo;
        }
    }

    void Vector3GizmoParameterEditor::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<Vector3GizmoParameterEditor, ValueParameterEditor>()
            ->Version(1)
            ->Field("value", &Vector3GizmoParameterEditor::m_currentValue)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<Vector3GizmoParameterEditor>("Vector3 gizmo parameter editor", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::Default, &Vector3GizmoParameterEditor::m_currentValue, "", "")
                ->Attribute(AZ::Edit::Attributes::DescriptionTextOverride, &ValueParameterEditor::GetDescription)
                ->Attribute(AZ::Edit::Attributes::Min, &Vector3GizmoParameterEditor::GetMinValue)
                ->Attribute(AZ::Edit::Attributes::Max, &Vector3GizmoParameterEditor::GetMaxValue)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &Vector3GizmoParameterEditor::OnValueChanged)
                ->Attribute(AZ::Edit::Attributes::ReadOnly, &ValueParameterEditor::IsReadOnly)
        ;
    }

    void Vector3GizmoParameterEditor::UpdateValue()
    {
        // Use the value from the first attribute, they all should match since they all are the same parameter in different graph instances
        if (!m_attributes.empty())
        {
            MCore::AttributeVector3* attribute = static_cast<MCore::AttributeVector3*>(m_attributes[0]);
            m_currentValue = AZ::Vector3(attribute->GetValue().GetX(), attribute->GetValue().GetY(), attribute->GetValue().GetZ());
        }
        else
        {
            const EMotionFX::Vector3Parameter* parameter = static_cast<const EMotionFX::Vector3Parameter*>(m_valueParameter);
            m_currentValue = parameter->GetDefaultValue();
        }
    }

    void Vector3GizmoParameterEditor::setIsReadOnly(bool isReadOnly)
    {
        ValueParameterEditor::setIsReadOnly(isReadOnly);
        if (m_gizmoButton)
        {
            m_gizmoButton->setEnabled(!IsReadOnly());
        }
    }

    QWidget* Vector3GizmoParameterEditor::CreateGizmoWidget(const AZStd::function<void()>& manipulatorCallback)
    {
        m_gizmoButton = new QPushButton();
        EMStudioManager::MakeTransparentButton(m_gizmoButton, "Images/Icons/Vector3GizmoDisabled.png", "Show/Hide translation gizmo for visual manipulation");
        QObject::connect(m_gizmoButton, &QPushButton::clicked, [this]() { ToggleTranslationGizmo(); });
        m_gizmoButton->setCheckable(true);
        m_gizmoButton->setEnabled(!IsReadOnly());
        m_manipulatorCallback = manipulatorCallback;
        return m_gizmoButton;
    }

    void Vector3GizmoParameterEditor::SetValue(const AZ::Vector3& value)
    {
        m_currentValue = value;
        UpdateAnimGraphInstanceAttributes();
    }

    AZ::Vector3 Vector3GizmoParameterEditor::GetMinValue() const
    {
        const EMotionFX::Vector3Parameter* parameter = static_cast<const EMotionFX::Vector3Parameter*>(m_valueParameter);
        return parameter->GetMinValue();
    }

    AZ::Vector3 Vector3GizmoParameterEditor::GetMaxValue() const
    {
        const EMotionFX::Vector3Parameter* parameter = static_cast<const EMotionFX::Vector3Parameter*>(m_valueParameter);
        return parameter->GetMaxValue();
    }

    void Vector3GizmoParameterEditor::OnValueChanged()
    {
        UpdateAnimGraphInstanceAttributes();

        if (m_transformationGizmo)
        {
            m_transformationGizmo->SetRenderOffset(m_currentValue);
        }
    }

    void Vector3GizmoParameterEditor::UpdateAnimGraphInstanceAttributes()
    {
        for (MCore::Attribute* attribute : m_attributes)
        {
            MCore::AttributeVector3* typedAttribute = static_cast<MCore::AttributeVector3*>(attribute);
            typedAttribute->SetValue(AZ::PackedVector3f(m_currentValue.GetX(), m_currentValue.GetY(), m_currentValue.GetZ()));
        }
    }

    class GizmoCallback
        : public MCommon::ManipulatorCallback
    {
    public:
        GizmoCallback(const AZStd::function<void()>& manipulatorCallback, const AZ::Vector3& oldValue, Vector3GizmoParameterEditor* parentEditor = nullptr)
            : MCommon::ManipulatorCallback(nullptr, oldValue)
            , m_parentEditor(parentEditor)
            , m_manipulatorCallback(manipulatorCallback)
        {}

        void Update(const AZ::Vector3& value) override
        {
            // call the base class update function
            MCommon::ManipulatorCallback::Update(value);

            // update the value of the attribute
            m_parentEditor->SetValue(value);

            if (m_manipulatorCallback)
            {
                m_manipulatorCallback();
            }
        }

    private:
        Vector3GizmoParameterEditor* m_parentEditor;
        const AZStd::function<void()>& m_manipulatorCallback;
    };

    void Vector3GizmoParameterEditor::ToggleTranslationGizmo()
    {
        if (m_gizmoButton->isChecked())
        {
            EMStudioManager::MakeTransparentButton(m_gizmoButton, "Images/Icons/Vector3Gizmo.png", "Show/Hide translation gizmo for visual manipulation");
        }
        else
        {
            EMStudioManager::MakeTransparentButton(m_gizmoButton, "Images/Icons/Vector3GizmoDisabled.png", "Show/Hide translation gizmo for visual manipulation");
        }

        if (!m_transformationGizmo)
        {
            m_transformationGizmo = static_cast<MCommon::TranslateManipulator*>(GetManager()->AddTransformationManipulator(new MCommon::TranslateManipulator(70.0f, true)));
            m_transformationGizmo->Init(m_currentValue);
            m_transformationGizmo->SetCallback(new GizmoCallback(m_manipulatorCallback, m_currentValue, this));
            m_transformationGizmo->SetName(m_valueParameter->GetName());
        }
        else
        {
            GetManager()->RemoveTransformationManipulator(m_transformationGizmo);
            delete m_transformationGizmo;
            m_transformationGizmo = nullptr;
        }
    }
}
