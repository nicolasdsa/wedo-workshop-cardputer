from __future__ import annotations
from enum import Enum

from pydantic import BaseModel, ConfigDict, Field


class ScenarioScreenType(str, Enum):
    TITLE_IMAGE_TEXT = "title_image_text"
    TEXT_GIF_BUTTON = "text_gif_button"
    TITLE_IMAGE_SLIDER = "title_image_slider"
    CANVAS = "canvas"


class ScenarioScreenSliderImageBase(BaseModel):
    order_index: int
    image_path: str
    caption: str | None = None


class ScenarioScreenSliderImageCreate(ScenarioScreenSliderImageBase):
    pass


class ScenarioScreenSliderImageRead(ScenarioScreenSliderImageBase):
    id: int

    model_config = ConfigDict(from_attributes=True)


class ScenarioScreenComponentBase(BaseModel):
    component_type: str
    x: float
    y: float
    w: float
    h: float
    rotate: float | None = None
    z_index: int = 0
    props: dict | list | str | None = None


class ScenarioScreenComponentCreate(ScenarioScreenComponentBase):
    pass


class ScenarioScreenComponentRead(ScenarioScreenComponentBase):
    id: int
    screen_id: int

    model_config = ConfigDict(from_attributes=True)


class ScenarioScreenBase(BaseModel):
    order_index: int
    screen_type: ScenarioScreenType | None = ScenarioScreenType.CANVAS
    title: str | None = None
    body_text: str | None = None
    image_path: str | None = None
    gif_path: str | None = None
    button_label: str | None = None
    animation_key: str | None = None


class ScenarioScreenCreate(ScenarioScreenBase):
    slider_images: list[ScenarioScreenSliderImageCreate] | None = None
    components: list[ScenarioScreenComponentCreate] = Field(default_factory=list)


class ScenarioScreenRead(ScenarioScreenBase):
    id: int
    slider_images: list[ScenarioScreenSliderImageRead] = Field(default_factory=list)
    components: list[ScenarioScreenComponentRead] = Field(default_factory=list)

    model_config = ConfigDict(from_attributes=True)
