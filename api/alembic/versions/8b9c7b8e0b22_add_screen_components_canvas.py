"""Add canvas-based screen components

Revision ID: 8b9c7b8e0b22
Revises: 989e4819d21b
Create Date: 2025-03-03 00:00:00.000000

"""
from __future__ import annotations

from collections import defaultdict
from typing import Sequence, Union

from alembic import op
import sqlalchemy as sa
from sqlalchemy.dialects import postgresql

# revision identifiers, used by Alembic.
revision: str = "8b9c7b8e0b22"
down_revision: Union[str, Sequence[str], None] = "989e4819d21b"
branch_labels: Union[str, Sequence[str], None] = None
depends_on: Union[str, Sequence[str], None] = None


def _props_column():
    return sa.JSON().with_variant(
        postgresql.JSONB(astext_type=sa.Text()), "postgresql"
    )


def upgrade() -> None:
    op.create_table(
        "scenario_screen_components",
        sa.Column("id", sa.Integer(), nullable=False),
        sa.Column("screen_id", sa.Integer(), nullable=False),
        sa.Column("component_type", sa.String(length=50), nullable=False),
        sa.Column("z_index", sa.Integer(), server_default="0", nullable=False),
        sa.Column("x", sa.Float(), nullable=False),
        sa.Column("y", sa.Float(), nullable=False),
        sa.Column("w", sa.Float(), nullable=False),
        sa.Column("h", sa.Float(), nullable=False),
        sa.Column("rotate", sa.Float(), nullable=True),
        sa.Column("props", _props_column(), nullable=True),
        sa.ForeignKeyConstraint(
            ["screen_id"], ["scenario_screens.id"], ondelete="CASCADE"
        ),
        sa.PrimaryKeyConstraint("id"),
    )
    op.create_index(
        op.f("ix_scenario_screen_components_id"),
        "scenario_screen_components",
        ["id"],
        unique=False,
    )

    connection = op.get_bind()

    screens_table = sa.table(
        "scenario_screens",
        sa.column("id", sa.Integer),
        sa.column("screen_type", sa.String),
        sa.column("title", sa.String),
        sa.column("body_text", sa.Text),
        sa.column("image_path", sa.String),
        sa.column("gif_path", sa.String),
        sa.column("button_label", sa.String),
    )

    slider_images_table = sa.table(
        "scenario_screen_slider_images",
        sa.column("id", sa.Integer),
        sa.column("screen_id", sa.Integer),
        sa.column("order_index", sa.Integer),
        sa.column("image_path", sa.String),
        sa.column("caption", sa.Text),
    )

    components_table = sa.table(
        "scenario_screen_components",
        sa.column("id", sa.Integer),
        sa.column("screen_id", sa.Integer),
        sa.column("component_type", sa.String),
        sa.column("z_index", sa.Integer),
        sa.column("x", sa.Float),
        sa.column("y", sa.Float),
        sa.column("w", sa.Float),
        sa.column("h", sa.Float),
        sa.column("rotate", sa.Float),
        sa.column("props", _props_column()),
    )

    screens = connection.execute(sa.select(screens_table)).fetchall()
    slider_images = connection.execute(
        sa.select(slider_images_table).order_by(slider_images_table.c.order_index)
    ).fetchall()

    sliders_by_screen: dict[int, list] = defaultdict(list)
    for image in slider_images:
        sliders_by_screen[image.screen_id].append(image)

    def add_component(row, component_type, layout, props=None, z_index=None):
        connection.execute(
            sa.insert(components_table).values(
                screen_id=row.id,
                component_type=component_type,
                z_index=z_index if z_index is not None else layout.get("z", 0),
                x=layout.get("x", 0.0),
                y=layout.get("y", 0.0),
                w=layout.get("w", 100.0),
                h=layout.get("h", 100.0),
                rotate=layout.get("rotate"),
                props=props or None,
            )
        )

    for screen in screens:
        if not screen.screen_type:
            continue

        z = 0
        if screen.screen_type == "title_image_text":
            if screen.title:
                add_component(
                    screen,
                    "text",
                    {"x": 10.0, "y": 6.0, "w": 80.0, "h": 14.0, "z": z},
                    {"text": screen.title, "variant": "heading"},
                )
                z += 1
            if screen.image_path:
                add_component(
                    screen,
                    "image",
                    {"x": 20.0, "y": 22.0, "w": 60.0, "h": 42.0, "z": z},
                    {"src": screen.image_path, "alt": screen.title or "Imagem"},
                )
                z += 1
            if screen.body_text:
                add_component(
                    screen,
                    "text",
                    {"x": 10.0, "y": 68.0, "w": 80.0, "h": 22.0, "z": z},
                    {"text": screen.body_text, "variant": "body"},
                )
                z += 1

        elif screen.screen_type == "text_gif_button":
            if screen.body_text:
                add_component(
                    screen,
                    "text",
                    {"x": 12.0, "y": 10.0, "w": 76.0, "h": 32.0, "z": z},
                    {"text": screen.body_text, "variant": "body"},
                )
                z += 1
            if screen.gif_path:
                add_component(
                    screen,
                    "gif",
                    {"x": 24.0, "y": 46.0, "w": 52.0, "h": 36.0, "z": z},
                    {"src": screen.gif_path},
                )
                z += 1
            label = screen.button_label or "Próximo"
            add_component(
                screen,
                "button",
                {"x": 42.0, "y": 86.0, "w": 16.0, "h": 10.0, "z": z},
                {"label": label, "action": {"type": "next"}},
            )

        elif screen.screen_type == "title_image_slider":
            if screen.title:
                add_component(
                    screen,
                    "text",
                    {"x": 12.0, "y": 6.0, "w": 76.0, "h": 12.0, "z": z},
                    {"text": screen.title, "variant": "heading"},
                )
                z += 1
            if screen.body_text:
                add_component(
                    screen,
                    "text",
                    {"x": 12.0, "y": 18.0, "w": 76.0, "h": 16.0, "z": z},
                    {"text": screen.body_text, "variant": "body"},
                )
                z += 1

            items = [
                {"image_path": img.image_path, "caption": img.caption}
                for img in sliders_by_screen.get(screen.id, [])
            ]
            add_component(
                screen,
                "gallery",
                {"x": 18.0, "y": 36.0, "w": 64.0, "h": 48.0, "z": z},
                {"items": items, "start_index": 0} if items else {"items": []},
            )
            z += 1

        else:
            # For any unknown type, keep a placeholder to avoid blank screens.
            add_component(
                screen,
                "text",
                {"x": 10.0, "y": 10.0, "w": 80.0, "h": 20.0, "z": z},
                {"text": "Tela convertida", "variant": "body"},
            )

        connection.execute(
            sa.update(screens_table)
            .where(screens_table.c.id == screen.id)
            .values(screen_type="canvas")
        )


def downgrade() -> None:
    op.drop_index(
        op.f("ix_scenario_screen_components_id"),
        table_name="scenario_screen_components",
    )
    op.drop_table("scenario_screen_components")
