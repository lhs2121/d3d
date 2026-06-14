#pragma once

#include <array>

class GameInventory
{
public:
	static constexpr int SlotCount = 56;
	static constexpr int EmptyItem = -1;

	GameInventory();

	void Clear();
	int GetSelectedSlot() const;
	void SetSelectedSlot(int slot);
	int GetDragSlot() const;
	void BeginDrag(int slot);
	void ClearDrag();

	int GetSlotItem(int slot) const;
	int GetSlotCount(int slot) const;
	int GetItemCount(int item) const;
	int AddItem(int item, int amount);
	bool RemoveItem(int item, int amount);
	bool ConsumeSelected(int amount = 1);
	void ClearSlotIfEmpty(int slot);
	void SwapSlots(int firstSlot, int secondSlot);

private:
	bool IsValidSlot(int slot) const;

	std::array<int, SlotCount> m_items = {};
	std::array<int, SlotCount> m_counts = {};
	int m_selectedSlot = 0;
	int m_dragSlot = -1;
};
