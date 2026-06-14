from __future__ import annotations

import html
import zipfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "assets" / "data" / "game_content.xlsx"


BLOCKS = [
    ("BlockGrass", 0, "풀 블록", 0, "초원 표면"),
    ("BlockDirt", 1, "흙 블록", 1, "초원 지층"),
    ("BlockStone", 2, "돌", 2, "기본 광물층"),
    ("BlockOre", 3, "광석", 3, "제작 재료"),
    ("BlockSand", 4, "모래", 4, "사막 표면"),
    ("BlockWood", 5, "나무", 5, "나무 자원"),
    ("BlockLeaves", 6, "잎", 6, "물약 재료"),
    ("BlockCraftingTable", 7, "작업대", 7, "제작 확장"),
    ("BlockPrairieStone", 8, "초원석", 2, "돌로 회수"),
    ("BlockMossStone", 9, "이끼돌", 2, "돌로 회수"),
    ("BlockSandstone", 10, "사암", 2, "돌로 회수"),
    ("BlockDesertStone", 11, "사막석", 2, "돌로 회수"),
    ("BlockSnow", 12, "눈", 4, "모래로 회수"),
    ("BlockIce", 13, "얼음", 2, "돌로 회수"),
    ("BlockFrozenStone", 14, "동토석", 2, "돌로 회수"),
    ("BlockCrystalOre", 15, "수정광석", 3, "광석으로 회수"),
    ("BlockPlacedWood", 16, "설치된 나무", 5, "나무로 회수"),
]


ITEMS = [
    (0, "SlotGrass", "풀 블록", "TerrainBlock", "", "", 99, 0, 0, 0, 0, 1, 0, 0, "", 0, 0, 0),
    (1, "SlotDirt", "흙 블록", "TerrainBlock", "", "", 99, 0, 0, 0, 0, 1, 0, 0, "", 0, 0, 0),
    (2, "SlotStone", "돌", "TerrainBlock", "", "", 99, 0, 0, 0, 0, 1, 0, 0, "", 0, 0, 0),
    (3, "SlotOre", "광석", "TerrainBlock", "", "", 99, 0, 0, 0, 0, 1, 0, 0, "", 0, 0, 0),
    (4, "SlotSand", "모래", "TerrainBlock", "", "", 99, 0, 0, 0, 0, 1, 0, 0, "", 0, 0, 0),
    (5, "SlotWood", "나무", "TerrainBlock", "", "", 99, 0, 0, 0, 0, 1, 0, 0, "", 0, 0, 0),
    (6, "SlotLeaves", "잎", "TerrainBlock", "", "", 99, 0, 0, 0, 0, 1, 0, 0, "", 0, 0, 0),
    (7, "SlotCraftingTable", "작업대", "Furniture", "", "", 99, 0, 0, 0, 0, 1, 0, 0, "", 0, 0, 0),
    (8, "SlotSword", "검", "Equipment", "Weapon", "", 1, 28, 0, 0, 0, 1, 0, 0, "", 0, 0, 0),
    (9, "SlotAxe", "도끼", "Equipment", "Tool", "", 1, 4, 0, 0, 0, 4.5, 0, 0, "", 0, 0, 0),
    (10, "SlotTeleportPotion", "텔레포트 물약", "Consumable", "", "텔레포트", 16, 0, 0, 0, 0, 1, 0, 0, "", 0, 0, 0),
    (11, "SlotHealthPotion", "HP 물약", "Consumable", "", "HP 회복", 16, 0, 0, 0, 0, 1, 35, 0, "", 0, 0, 0),
    (12, "SlotGreaterHealthPotion", "대형 HP 물약", "Consumable", "", "대형 HP 회복", 8, 0, 0, 0, 0, 1, 70, 0, "", 0, 0, 0),
    (13, "SlotSpeedPotion", "속도 물약", "Consumable", "", "속도 강화", 8, 0, 0, 1.8, 0, 1, 0, 18, "", 0, 0, 0),
    (14, "SlotJumpPotion", "점프 물약", "Consumable", "", "점프 강화", 8, 0, 0, 0, 4, 1, 0, 18, "", 0, 0, 0),
    (15, "SlotGuardPotion", "방어 물약", "Consumable", "", "방어 강화", 8, 0, 4, 0, 0, 1, 0, 22, "", 0, 0, 0),
    (16, "SlotIronHelmet", "철 투구", "Equipment", "Helmet", "", 1, 0, 2, 0, 0, 1, 0, 0, "", 0, 0, 0),
    (17, "SlotIronArmor", "철 갑옷", "Equipment", "Armor", "", 1, 0, 5, 0, 0, 1, 0, 0, "", 0, 0, 0),
    (18, "SlotSwiftBoots", "신속의 신발", "Equipment", "Boots", "", 1, 0, 1, 1.2, 0.6, 1, 0, 0, "", 0, 0, 0),
    (19, "SlotLuckyCharm", "행운 부적", "Equipment", "Accessory", "", 1, 2, 1, 0, 0, 1, 0, 0, "", 0, 0, 0),
    (20, "SlotPistol", "권총", "Equipment", "Weapon", "", 1, 8, 0, 0, 0, 1, 0, 0, "SlotBullet", 1, 34, 22),
    (21, "SlotBullet", "총알", "Ammo", "", "", 99, 0, 0, 0, 0, 1, 0, 0, "", 0, 0, 0),
    (22, "SlotBow", "활", "Equipment", "Weapon", "", 1, 6, 0, 0, 0, 1, 0, 0, "SlotArrow", 1, 24, 16),
    (23, "SlotArrow", "화살", "Ammo", "", "", 99, 0, 0, 0, 0, 1, 0, 0, "", 0, 0, 0),
]


RECIPES = [
    ("작업대 제작", "SlotCraftingTable", 1, "no", "SlotWood", 4, "", 0, "", 0, "", 0),
    ("검 제작", "SlotSword", 1, "yes", "SlotWood", 2, "SlotStone", 3, "", 0, "", 0),
    ("도끼 제작", "SlotAxe", 1, "yes", "SlotWood", 3, "SlotStone", 2, "", 0, "", 0),
    ("HP 물약 제작", "SlotHealthPotion", 1, "yes", "SlotLeaves", 2, "SlotSand", 1, "", 0, "", 0),
    ("대형 HP 물약 제작", "SlotGreaterHealthPotion", 1, "yes", "SlotHealthPotion", 2, "SlotOre", 1, "", 0, "", 0),
    ("속도 물약 제작", "SlotSpeedPotion", 1, "yes", "SlotLeaves", 3, "SlotOre", 1, "", 0, "", 0),
    ("점프 물약 제작", "SlotJumpPotion", 1, "yes", "SlotLeaves", 2, "SlotSand", 2, "", 0, "", 0),
    ("방어 물약 제작", "SlotGuardPotion", 1, "yes", "SlotStone", 2, "SlotOre", 1, "", 0, "", 0),
    ("철 투구 제작", "SlotIronHelmet", 1, "yes", "SlotOre", 4, "", 0, "", 0, "", 0),
    ("철 갑옷 제작", "SlotIronArmor", 1, "yes", "SlotOre", 8, "", 0, "", 0, "", 0),
    ("신속의 신발 제작", "SlotSwiftBoots", 1, "yes", "SlotOre", 3, "SlotSand", 2, "", 0, "", 0),
    ("행운 부적 제작", "SlotLuckyCharm", 1, "yes", "SlotOre", 3, "SlotLeaves", 3, "", 0, "", 0),
    ("권총 제작", "SlotPistol", 1, "yes", "SlotOre", 6, "SlotWood", 2, "", 0, "", 0),
    ("총알 제작", "SlotBullet", 12, "yes", "SlotOre", 1, "", 0, "", 0, "", 0),
    ("활 제작", "SlotBow", 1, "yes", "SlotWood", 3, "SlotLeaves", 2, "", 0, "", 0),
    ("화살 제작", "SlotArrow", 8, "yes", "SlotWood", 1, "SlotStone", 1, "", 0, "", 0),
]


def col_name(index: int) -> str:
    name = ""
    while index:
        index, remainder = divmod(index - 1, 26)
        name = chr(65 + remainder) + name
    return name


def cell_xml(value, row_index: int, col_index: int) -> str:
    ref = f"{col_name(col_index)}{row_index}"
    if isinstance(value, (int, float)) and not isinstance(value, bool):
        return f'<c r="{ref}"><v>{value}</v></c>'
    text = html.escape(str(value), quote=False)
    return f'<c r="{ref}" t="inlineStr"><is><t>{text}</t></is></c>'


def sheet_xml(rows: list[list[object]]) -> str:
    row_xml = []
    for row_index, row in enumerate(rows, start=1):
        cells = "".join(cell_xml(value, row_index, col_index) for col_index, value in enumerate(row, start=1))
        row_xml.append(f'<row r="{row_index}">{cells}</row>')
    return (
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
        '<worksheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main">'
        '<sheetData>'
        + "".join(row_xml)
        + '</sheetData></worksheet>'
    )


def write_workbook() -> None:
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    sheets = [
        ("Blocks", [["constant", "id", "name_ko", "inventory_item_id", "notes"], *BLOCKS]),
        (
            "Items",
            [
                [
                    "id",
                    "constant",
                    "name_ko",
                    "kind",
                    "equip_slot",
                    "action_label",
                    "max_stack",
                    "attack",
                    "defense",
                    "move_bonus",
                    "jump_bonus",
                    "chop_multiplier",
                    "heal",
                    "duration_sec",
                    "required_item",
                    "required_amount",
                    "action_damage",
                    "range_tiles",
                ],
                *ITEMS,
            ],
        ),
        (
            "Recipes",
            [
                [
                    "action",
                    "output_item",
                    "output_amount",
                    "requires_table",
                    "ingredient_1",
                    "amount_1",
                    "ingredient_2",
                    "amount_2",
                    "ingredient_3",
                    "amount_3",
                    "ingredient_4",
                    "amount_4",
                ],
                *RECIPES,
            ],
        ),
    ]

    workbook_sheets = "".join(
        f'<sheet name="{name}" sheetId="{index}" r:id="rId{index}"/>'
        for index, (name, _) in enumerate(sheets, start=1)
    )
    workbook_rels = "".join(
        f'<Relationship Id="rId{index}" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet" Target="worksheets/sheet{index}.xml"/>'
        for index, _ in enumerate(sheets, start=1)
    )
    content_types = "".join(
        f'<Override PartName="/xl/worksheets/sheet{index}.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml"/>'
        for index, _ in enumerate(sheets, start=1)
    )

    with zipfile.ZipFile(OUTPUT, "w", zipfile.ZIP_DEFLATED) as workbook:
        workbook.writestr(
            "[Content_Types].xml",
            '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
            '<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">'
            '<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>'
            '<Default Extension="xml" ContentType="application/xml"/>'
            '<Override PartName="/xl/workbook.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml"/>'
            + content_types
            + '</Types>',
        )
        workbook.writestr(
            "_rels/.rels",
            '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
            '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
            '<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="xl/workbook.xml"/>'
            '</Relationships>',
        )
        workbook.writestr(
            "xl/workbook.xml",
            '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
            '<workbook xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main" '
            'xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">'
            '<sheets>'
            + workbook_sheets
            + '</sheets></workbook>',
        )
        workbook.writestr(
            "xl/_rels/workbook.xml.rels",
            '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
            '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
            + workbook_rels
            + '</Relationships>',
        )
        for index, (_, rows) in enumerate(sheets, start=1):
            workbook.writestr(f"xl/worksheets/sheet{index}.xml", sheet_xml(rows))


if __name__ == "__main__":
    write_workbook()
    print(OUTPUT)
