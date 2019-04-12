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

#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>
#include <AzToolsFramework/AssetBrowser/Views/EntryDelegate.h>

#include <QApplication>
#include <QPainter>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        const int ENTRY_SPACING_LEFT_PIXELS = 8;
        const int ENTRY_ICON_MARGIN_LEFT_PIXELS = 2;

        EntryDelegate::EntryDelegate(QWidget* parent)
            : QStyledItemDelegate(parent)
            , m_iconSize(qApp->style()->pixelMetric(QStyle::PM_SmallIconSize))
        {
        }

        EntryDelegate::~EntryDelegate() = default;

        QSize EntryDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
        {
            QSize baseHint = QStyledItemDelegate::sizeHint(option, index);
            if (baseHint.height() < m_iconSize)
            {
                baseHint.setHeight(m_iconSize);
            }
            return baseHint;
        }

        void EntryDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
        {
            auto data = index.data(AssetBrowserModel::Roles::EntryRole);
            if (data.canConvert<const AssetBrowserEntry*>())
            {
                bool isEnabled = option.state & QStyle::State_Enabled;
                bool isSelected = option.state & QStyle::State_Selected;

                QStyle* style = option.widget ? option.widget->style() : QApplication::style();

                if (isSelected)
                {
                    painter->fillRect(option.rect, option.palette.highlight());
                }

                auto entry = qvariant_cast<const AssetBrowserEntry*>(data);

                // Draw main entry thumbnail.
                QRect remainingRect(option.rect);
                remainingRect.adjust(ENTRY_ICON_MARGIN_LEFT_PIXELS, 0, 0, 0); // bump it rightwards to give some margin to the icon.

                QSize iconSize(m_iconSize, m_iconSize);
                // Note that the thumbnail might actually be smaller than the row if theres a lot of padding or font size
                // so it needs to center vertically with padding in that case:
                QPoint iconTopLeft(remainingRect.x(), remainingRect.y() + (remainingRect.height() / 2) - (m_iconSize / 2));

                auto sourceEntry = azrtti_cast<const SourceAssetBrowserEntry*>(entry);

                int thumbX = DrawThumbnail(painter, iconTopLeft, iconSize, entry->GetThumbnailKey());

                QPalette actualPalette(option.palette);

                if (sourceEntry)
                {
                    if (m_showSourceControl)
                    {
                        DrawThumbnail(painter, iconTopLeft, iconSize, sourceEntry->GetSourceControlThumbnailKey());
                    }
                    // sources with no children should be greyed out.
                    if (sourceEntry->GetChildCount() == 0)
                    {
                        isEnabled = false; // draw in disabled style.
                        actualPalette.setCurrentColorGroup(QPalette::Disabled);
                    }
                }

                remainingRect.adjust(thumbX, 0, 0, 0); // bump it to the right by the size of the thumbnail
                remainingRect.adjust(ENTRY_SPACING_LEFT_PIXELS, 0, 0, 0); // bump it to the right by the spacing.

                style->drawItemText(painter,
                    remainingRect,
                    option.displayAlignment,
                    actualPalette,
                    isEnabled,
                    entry->GetDisplayName(),
                    isSelected ? QPalette::HighlightedText : QPalette::Text);
            }
        }

        void EntryDelegate::SetThumbnailContext(const char* thumbnailContext)
        {
            m_thumbnailContext = thumbnailContext;
        }

        void EntryDelegate::SetShowSourceControlIcons(bool showSourceControl)
        {
            m_showSourceControl = showSourceControl;
        }

        int EntryDelegate::DrawThumbnail(QPainter* painter, const QPoint& point, const QSize& size, Thumbnailer::SharedThumbnailKey thumbnailKey) const
        {
            SharedThumbnail thumbnail;
            ThumbnailerRequestsBus::BroadcastResult(thumbnail, &ThumbnailerRequests::GetThumbnail, thumbnailKey, m_thumbnailContext.c_str());
            AZ_Assert(thumbnail, "Could not get thumbnail");
            if (thumbnail->GetState() == Thumbnail::State::Failed)
            {
                return 0;
            }
            QPixmap pixmap = thumbnail->GetPixmap();
            painter->drawPixmap(point.x(), point.y(), size.width(), size.height(), pixmap);
            return m_iconSize;
        }
    } // namespace Thumbnailer
} // namespace AzToolsFramework

#include <AssetBrowser/Views/EntryDelegate.moc>
