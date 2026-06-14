#include "pch.h"
#include "GameInventory.h"

#include <algorithm>

GameInventory::GameInventory()
{
	Clear();
}

void GameInventory::Clear()
{
	m_items.fill(EmptyItem);
	m_counts.fill(0);
	m_selectedSlot = 0;
	m_dragSlot = -1;
}

int GameInventory::GetSelectedSlot() const
{
	return m_selectedSlot;
}

void GameInventory::SetSelectedSlot(int slot)
{
	if (IsValidSlot(slot))
		m_selectedSlot = slot;
}

int GameInventory::GetDragSlot() const
{
	return m_dragSlot;
}

void GameInventory::BeginDrag(int slot)
{
	m_dragSlot = IsValidSlot(slot) ? slot : -1;
}

void GameInventory::ClearDrag()
{
	m_dragSlot = -1;
}

int GameInventory::GetSlotItem(int slot) const
{
	if (!IsValidSlot(slot) || m_counts[slot] <= 0)
		return EmptyItem;

	return m_items[slot];
}

int GameInventory::GetSlotCount(int slot) const
{
	return IsValidSlot(slot) ? m_counts[slot] : 0;
}

int GameInventory::GetItemCount(int item) const
{
	if (item == EmptyItem)
		return 0;

	int total = 0;
	for (int slot = 0; slot < SlotCount; ++slot)
	{
		if (GetSlotItem(slot) == item)
			total += m_counts[slot];
	}
	return total;
}

int GameInventory::AddItem(int item, int amount)
{
	if (item == EmptyItem || amount <= 0)
		return -1;

	for (int slot = 0; slot < SlotCount; ++slot)
	{
		if (GetSlotItem(slot) == item)
		{
			m_counts[slot] += amount;
			return slot;
		}
	}

	for (int slot = 0; slot < SlotCount; ++slot)
	{
		if (GetSlotItem(slot) != EmptyItem)
			continue;

		m_items[slot] = item;
		m_counts[slot] = amount;
		return slot;
	}

	return -1;
}

bool GameInventory::RemoveItem(int item, int amount)
{
	if (amount <= 0)
		return true;
	if (GetItemCount(item) < amount)
		return false;

	int remaining = amount;
	for (int slot = 0; slot < SlotCount && remaining > 0; ++slot)
	{
		if (GetSlotItem(slot) != item)
			continue;

		const int removed = (std::min)(m_counts[slot], remaining);
		m_counts[slot] -= removed;
		remaining -= removed;
		ClearSlotIfEmpty(slot);
	}

	return remaining == 0;
}

bool GameInventory::ConsumeSelected(int amount)
{
	if (!IsValidSlot(m_selectedSlot) || amount <= 0 || m_counts[m_selectedSlot] < amount)
		return false;

	m_counts[m_selectedSlot] -= amount;
	ClearSlotIfEmpty(m_selectedSlot);
	return true;
}

void GameInventory::ClearSlotIfEmpty(int slot)
{
	if (!IsValidSlot(slot) || m_counts[slot] > 0)
		return;

	m_counts[slot] = 0;
	m_items[slot] = EmptyItem;
	if (m_dragSlot == slot)
		m_dragSlot = -1;
}

void GameInventory::SwapSlots(int firstSlot, int secondSlot)
{
	if (!IsValidSlot(firstSlot) || !IsValidSlot(secondSlot) || firstSlot == secondSlot)
		return;

	std::swap(m_items[firstSlot], m_items[secondSlot]);
	std::swap(m_counts[firstSlot], m_counts[secondSlot]);
	ClearSlotIfEmpty(firstSlot);
	ClearSlotIfEmpty(secondSlot);
}

bool GameInventory::IsValidSlot(int slot) const
{
	return slot >= 0 && slot < SlotCount;
}
