from __future__ import annotations
from typing import TYPE_CHECKING

from sqlalchemy import Float, ForeignKey, Integer, String
from sqlalchemy.dialects import postgresql
from sqlalchemy.orm import Mapped, mapped_column, relationship

from core.database import Base

if TYPE_CHECKING:
    from models.scenario_screen import ScenarioScreen


class ScenarioScreenComponent(Base):
    __tablename__ = "scenario_screen_components"

    id: Mapped[int] = mapped_column(primary_key=True, index=True)
    screen_id: Mapped[int] = mapped_column(
        ForeignKey("scenario_screens.id", ondelete="CASCADE"), nullable=False
    )
    component_type: Mapped[str] = mapped_column(String(50), nullable=False)
    z_index: Mapped[int] = mapped_column(Integer, nullable=False, default=0)
    x: Mapped[float] = mapped_column(Float, nullable=False)
    y: Mapped[float] = mapped_column(Float, nullable=False)
    w: Mapped[float] = mapped_column(Float, nullable=False)
    h: Mapped[float] = mapped_column(Float, nullable=False)
    rotate: Mapped[float | None] = mapped_column(Float, nullable=True)
    props: Mapped[dict | list | str | None] = mapped_column(
        postgresql.JSONB, nullable=True
    )

    screen: Mapped[ScenarioScreen] = relationship(back_populates="components")
