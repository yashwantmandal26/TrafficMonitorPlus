#pragma once
#include "stdafx.h"
#include <memory>
#include "IDrawCommon.h"
#include "DrawCommon.h"
#include "TaskBarDlgDrawCommon.h"
#include "DrawCommonHelper.h"

template <class T>
struct StackObjectDeleter
{
    using pointer = std::add_pointer_t<T>;
    void operator()(pointer p_stack_object) const noexcept
    {
        if (p_stack_object)
        {
            Destroy(p_stack_object);
        }
    }
};
using UniqueIDrawCommon = std::unique_ptr<void, StackObjectDeleter<IDrawCommon>>;
using UniqueIDrawBuffer = std::unique_ptr<void, StackObjectDeleter<IDrawBuffer>>;

/**
 * @brief After adding the renderer type, write the new type into the template parameters of the following types
 *
 */
using DrawCommonUnionStorage = AlignedUnionStorage<CDrawCommon, CTaskBarDlgDrawCommon>;
/**
 * @brief After adding the buffer type (that is, the object that submits the window drawing content to the system during destruction), write the new type into the template parameters of the following types
 *
 */
using DrawBufferUnionStorage = AlignedUnionStorage<CDrawDoubleBuffer, CTaskBarDlgDrawBuffer, CTaskBarDlgDrawBufferUseDComposition>;
struct InvolvedDrawCommonStorages
{
    DrawBufferUnionStorage m_draw_buffer_union_storage{};
    DrawCommonUnionStorage m_draw_common_union_storage{};
};
/**
 * @brief A stack memory block with DrawCommon and DrawBuffer stack memory and their corresponding exclusive pointers,
 Note: It will destroy DrawCommon first and then DrawBuffer.
 *
 */
struct AllInvolvedDrawCommonObjectsStorage
{
    InvolvedDrawCommonStorages m_storage{};
    // Destruct DrawCommon first and then DrawBuffer
    UniqueIDrawBuffer m_unique_draw_buffer{};
    UniqueIDrawCommon m_unique_draw_common{};

    AllInvolvedDrawCommonObjectsStorage() = default;
    ~AllInvolvedDrawCommonObjectsStorage() = default;
    // Disable copy move
    AllInvolvedDrawCommonObjectsStorage(const AllInvolvedDrawCommonObjectsStorage&) = delete;
    AllInvolvedDrawCommonObjectsStorage& operator=(const AllInvolvedDrawCommonObjectsStorage&) = delete;
};
using AllInvolvedDrawCommonObjectsInitializer = std::function<void(IDrawBuffer*, IDrawCommon*)>;
using TaggedAllInvolvedDrawCommonObjectsInitializer =
    std::pair<DrawCommonHelper::RenderType, AllInvolvedDrawCommonObjectsInitializer>;

/**
 * @brief Initialize the renderer and buffer using the function corresponding to RenderType from the stack memory
 *
 * @param ref_object_storage stack memory object containing a pointer with exclusive ownership of the memory
 * @param render_type renderer type enumeration
 * @param initializer_list The total collection of renderer type enumerations and corresponding initialization functions
 * @return std::tuple<IDrawBuffer*, IDrawCommon*> raw pointer without ownership
 */
auto GetInterfaceFromAllInvolvedDrawCommonObjects(
    AllInvolvedDrawCommonObjectsStorage& ref_object_storage,
    DrawCommonHelper::RenderType render_type,
    std::initializer_list<TaggedAllInvolvedDrawCommonObjectsInitializer> initializer_list)
    -> std::tuple<IDrawBuffer*, IDrawCommon*>;
