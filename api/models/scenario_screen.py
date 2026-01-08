from __future__ import annotations
from enum import Enum
from typing import TYPE_CHECKING

from sqlalchemy import ForeignKey, Integer, String, Text
from sqlalchemy.orm import Mapped, mapped_column, relationship

from core.database import Base

if TYPE_CHECKING:
    from models.scenario import Scenario
    from models.scenario_screen_slider_image import ScenarioScreenSliderImage
    from models.scenario_screen_component import ScenarioScreenComponent


class ScenarioScreenType(str, Enum):
    TITLE_IMAGE_TEXT = "title_image_text"
    TEXT_GIF_BUTTON = "text_gif_button"
    TITLE_IMAGE_SLIDER = "title_image_slider"
    CANVAS = "canvas"


class ScenarioScreen(Base):
    __tablename__ = "scenario_screens"

    id: Mapped[int] = mapped_column(primary_key=True, index=True)
    scenario_id: Mapped[int] = mapped_column(
        ForeignKey("scenarios.id", ondelete="CASCADE"), nullable=False
    )
    order_index: Mapped[int] = mapped_column(Integer, nullable=False)
    screen_type: Mapped[ScenarioScreenType] = mapped_column(
        String(50), nullable=False, default=ScenarioScreenType.CANVAS
    )
    # Legacy fields kept for backward compatibility with old payloads.
    title: Mapped[str | None] = mapped_column(String(255), nullable=True)
    body_text: Mapped[str | None] = mapped_column(Text, nullable=True)
    image_path: Mapped[str | None] = mapped_column(String(255), nullable=True)
    gif_path: Mapped[str | None] = mapped_column(String(255), nullable=True)
    button_label: Mapped[str | None] = mapped_column(String(100), nullable=True)
    animation_key: Mapped[str | None] = mapped_column(String(100), nullable=True)

    scenario: Mapped[Scenario] = relationship(back_populates="screens")
    slider_images: Mapped[list[ScenarioScreenSliderImage]] = relationship(
        back_populates="screen",
        cascade="all, delete-orphan",
        passive_deletes=True,
        order_by="ScenarioScreenSliderImage.order_index",
    )
    components: Mapped[list[ScenarioScreenComponent]] = relationship(
        back_populates="screen",
        cascade="all, delete-orphan",
        passive_deletes=True,
        order_by="ScenarioScreenComponent.z_index",
    )
