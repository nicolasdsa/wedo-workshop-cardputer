from fastapi import FastAPI
from fastapi.staticfiles import StaticFiles
from pathlib import Path

from core.config import get_settings
from core.error_handlers import register_exception_handlers
from routers import (
    health_router,
    instrument,
    reagent,
    scenario,
    reaction,
    scenario_run,
    ui_scenario,
    ui_scenario_run,
    artist,
    scenario_screen,
    ui_artist,
)

settings = get_settings()
app = FastAPI(title=settings.app_name)

base_dir = Path(__file__).resolve().parent
static_dir = base_dir / "static"
assets_dir = base_dir / "assets"

app.mount("/static", StaticFiles(directory=str(static_dir)), name="static")
app.mount("/assets", StaticFiles(directory=str(assets_dir)), name="assets")

app.include_router(health_router.router)
app.include_router(instrument.router)
app.include_router(reagent.router)
app.include_router(scenario.router)
app.include_router(reaction.router)
app.include_router(scenario_run.router)
app.include_router(ui_scenario.router)
app.include_router(ui_scenario_run.router)
app.include_router(artist.router)
app.include_router(scenario_screen.router)
app.include_router(ui_artist.router)
register_exception_handlers(app)
