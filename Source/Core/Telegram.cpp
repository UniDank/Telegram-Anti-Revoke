﻿#include "Telegram.h"

#include "Logger.h"
#include "Utils.h"
#include "IRuntime.h"
#include "IAntiRevoke.h"

//////////////////////////////////////////////////
// Object
//

int32_t Object::GetWidth()
{
    return this->MaxWidth;
}

void Object::SetWidth(int32_t Value)
{
    this->MaxWidth = Value;
}

//////////////////////////////////////////////////
// DocumentData
//

// DocumentType DocumentData::GetType()
// {
//     return *(DocumentType*)((uintptr_t)this + 8);
// }
//
// bool DocumentData::IsSticker()
// {
//     return GetType() == DocumentType::Sticker;
// }

//////////////////////////////////////////////////
// Media
//

// DocumentData* Media::GetDocument()
// {
//     // DocumentData *Media::document()
//     return *(DocumentData**)((uintptr_t)this + 8);
// }

//////////////////////////////////////////////////
// HistoryView::Element
//

Media *HistoryViewElement::GetMedia()
{
#if defined PLATFORM_X86
    return *(Media **)((uintptr_t)this + 0x24);
#elif defined PLATFORM_X64
    return *(Media **)((uintptr_t)this + 0x38);
#else
    #error "Unimplemented."
#endif
}

//////////////////////////////////////////////////
// HistoryMessageEdited
//

QtString *HistoryMessageEdited::GetTimeText()
{
#if defined PLATFORM_X86
    return (QtString *)((uintptr_t)this + 0x10);
#elif defined PLATFORM_X64
    return (QtString *)((uintptr_t)this + 0x18);
#else
    #error "Unimplemented."
#endif
}

//////////////////////////////////////////////////
// HistoryMessageSigned
//

QtString *HistoryMessageSigned::GetTimeText()
{
    return (QtString *)((uintptr_t)this + IRuntime::GetInstance().GetData().Offset.SignedTimeText);
}

//////////////////////////////////////////////////
// HistoryMessageReply
//

int32_t &HistoryMessageReply::MaxReplyWidth()
{
    return *(int32_t *)((uintptr_t)this + IRuntime::GetInstance().GetData().Offset.MaxReplyWidth);
}

//////////////////////////////////////////////////
// PeerData
//

// bool PeerData::IsChannel()
// {
//     return (GetId() & PeerIdTypeMask) == PeerIdChannelShift;
// }
//
// PeerData::PeerId PeerData::GetId()
// {
//     return *(PeerId*)((uintptr_t)this + 0x8);
// }

//////////////////////////////////////////////////
// History
//

// PeerData* History::GetPeer()
// {
//     return *(PeerData**)((uintptr_t)this + IRuntime::GetInstance().GetData().Offset.HistoryPeer);
// }

void History::OnDestroyMessage(HistoryMessage *pMessage)
{
    IAntiRevoke::GetInstance().OnDestroyMessage(this, pMessage);
}

//////////////////////////////////////////////////
// HistoryMessage
//

bool HistoryMessage::IsMessage()
{
    // Join channel msg is HistoryItem, that's not an inheritance class.
    // It will cause a memory access crash, so we need to filter it out.
    //

    auto RuntimeIns = IRuntime::GetInstance();
    const auto FileVersion = RuntimeIns.GetFileVersion();
    const auto &Index = RuntimeIns.GetData().Index;

    // ver < 3.2.5
    if (FileVersion < 3002005) {
        // HistoryMessage *HistoryItem::toHistoryMessage()
        //
        using FnToHistoryMessageT = HistoryMessage *(*)(HistoryMessage * This);
        return Utils::CallVirtual<FnToHistoryMessageT>(this, Index.ToHistoryMessage)(this) !=
               nullptr;
    }
    // ver >= 3.2.5
    else if (FileVersion >= 3002005) {
        // bool HistoryItem::isService() const
        //
        using FnIsServiceT = bool (*)(HistoryMessage * This);
        return !Utils::CallVirtual<FnIsServiceT>(this, Index.IsService)(this);
    }
    else {
        return false;
    }
}

template <class CompT>
CompT *HistoryMessage::GetComponent(uint32_t index)
{
    CompT *result = nullptr;

    Safe::TryExcept(
        [&]() {
            struct RuntimeComposerMetadata
            {
                size_t size;
                size_t align;
                size_t offsets[64];
                int last;
            };

            auto data = *(void ***)((uintptr_t)this + 8);
            auto metaData = (RuntimeComposerMetadata *)(*data);
            auto offset = metaData->offsets[index];
            if (offset >= sizeof(RuntimeComposerMetadata *)) {
                result = (CompT *)((uintptr_t)data + offset);
            }
        },
        [&](ULONG ExceptionCode) {
            LOG(Warn,
                "Function: [" __FUNCTION__ "] An exception was caught. Code: {:#x}, Address: {}",
                ExceptionCode, (void *)this);
        });

    return result;
}

HistoryMessageEdited *HistoryMessage::GetEdited()
{
    return GetComponent<HistoryMessageEdited>(
        IRuntime::GetInstance().GetData().Function.EditedIndex());
}

HistoryMessageSigned *HistoryMessage::GetSigned()
{
    return GetComponent<HistoryMessageSigned>(
        IRuntime::GetInstance().GetData().Function.SignedIndex());
}

HistoryMessageReply *HistoryMessage::GetReply()
{
    return GetComponent<HistoryMessageReply>(
        IRuntime::GetInstance().GetData().Function.ReplyIndex());
}

// History* HistoryMessage::GetHistory()
// {
//     return *(History**)((uintptr_t)this + 0x10);
// }

// Media* HistoryMessage::GetMedia()
// {
//     return *(Media**)((uintptr_t)this + IRuntime::GetInstance().GetData().Offset.Media);
// }
//
// bool HistoryMessage::IsSticker()
// {
//     if (Media *pMedia = GetMedia()) {
//         if (DocumentData *pData = pMedia->GetDocument()) {
//             return pData->IsSticker();
//         }
//     }
//     return FALSE;
// }

// bool HistoryMessage::IsLargeEmoji()
// {
//     // if it's a LargeEmoji, [Item->Media] is nullptr, and [Item->MainView->Media] isn't nullptr.
//     // if it's a Video, then [Item->Media] isn't nullptr.
//
//     Media *pMedia = GetMedia();
//     if (pMedia != nullptr) {
//         return FALSE;
//     }
//
//     HistoryViewElement *pMainView = GetMainView();
//     if (pMainView == nullptr) {
//         return FALSE;
//     }
//
//     return pMainView->GetMedia() != nullptr;
// }

HistoryViewElement *HistoryMessage::GetMainView()
{
    return *(
        HistoryViewElement **)((uintptr_t)this + IRuntime::GetInstance().GetData().Offset.MainView);
}

QtString *HistoryMessage::GetTimeText()
{
    return (QtString *)((uintptr_t)this + IRuntime::GetInstance().GetData().Offset.TimeText);
}

int32_t HistoryMessage::GetTimeWidth()
{
    return *(int32_t *)((uintptr_t)this + IRuntime::GetInstance().GetData().Offset.TimeWidth);
}
void HistoryMessage::SetTimeWidth(int32_t Value)
{
    *(int32_t *)((uintptr_t)this + IRuntime::GetInstance().GetData().Offset.TimeWidth) = Value;
}

//////////////////////////////////////////////////
// Lang::Instance
//

QtString *LanguageInstance::GetId()
{
#if defined PLATFORM_X86
    return (QtString *)((uintptr_t)this + 0x4);
#elif defined PLATFORM_X64
    return (QtString *)((uintptr_t)this + 0x8);
#else
    #error "Unimplemented."
#endif
}

QtString *LanguageInstance::GetPluralId()
{
#if defined PLATFORM_X86
    return (QtString *)((uintptr_t)this + 0x8);
#elif defined PLATFORM_X64
    return (QtString *)((uintptr_t)this + 0x10);
#else
    #error "Unimplemented."
#endif
}

QtString *LanguageInstance::GetName()
{
#if defined PLATFORM_X86
    return (QtString *)((uintptr_t)this + 0x14);
#elif defined PLATFORM_X64
    return (QtString *)((uintptr_t)this + 0x28);
#else
    #error "Unimplemented."
#endif
}

QtString *LanguageInstance::GetNativeName()
{
#if defined PLATFORM_X86
    return (QtString *)((uintptr_t)this + 0x18);
#elif defined PLATFORM_X64
    return (QtString *)((uintptr_t)this + 0x30);
#else
    #error "Unimplemented."
#endif
}
