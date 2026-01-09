from fastapi import APIRouter, Depends, Request
from fastapi.responses import HTMLResponse
from sqlalchemy.orm import Session

from controllers import ui_scenario
from core.dependencies import get_db

router = APIRouter(
    prefix="/ui/scenarios",
    tags=["ui-scenarios"],
    default_response_class=HTMLResponse,
)


@router.get("/", response_class=HTMLResponse)
def list_scenarios_page(request: Request, db: Session = Depends(get_db)):
    return ui_scenario.list_scenarios_page(request, db)


@router.get("/list", response_class=HTMLResponse)
def list_scenarios_partial(request: Request, db: Session = Depends(get_db)):
    return ui_scenario.list_scenarios_partial(request, request.query_params, db)


@router.get("/new-modal", response_class=HTMLResponse)
def new_scenario_modal(request: Request, db: Session = Depends(get_db)):
    return ui_scenario.new_scenario_modal(request, db)


@router.get("/{scenario_id}/edit-modal", response_class=HTMLResponse)
def edit_scenario_modal(
    scenario_id: int, request: Request, db: Session = Depends(get_db)
):
    return ui_scenario.edit_scenario_modal(scenario_id, request, db)


@router.get("/{scenario_id}/delete-modal", response_class=HTMLResponse)
def delete_scenario_modal(
    scenario_id: int, request: Request, db: Session = Depends(get_db)
):
    return ui_scenario.delete_scenario_modal(scenario_id, request, db)


@router.post("/", response_class=HTMLResponse)
async def create_scenario(request: Request, db: Session = Depends(get_db)):
    return await ui_scenario.create_scenario_action(request, db)


@router.post("/{scenario_id}", response_class=HTMLResponse)
async def update_scenario(
    scenario_id: int, request: Request, db: Session = Depends(get_db)
):
    return await ui_scenario.update_scenario_action(scenario_id, request, db)


@router.post("/{scenario_id}/delete", response_class=HTMLResponse)
async def delete_scenario(
    scenario_id: int, request: Request, db: Session = Depends(get_db)
):
    return await ui_scenario.delete_scenario_action(scenario_id, request, db)


@router.get("/{scenario_id}/run", response_class=HTMLResponse)
def run_scenario_page(scenario_id: int, request: Request, db: Session = Depends(get_db)):
    return ui_scenario.run_scenario_page(scenario_id, request, db)


@router.get("/{scenario_id}/builder", response_class=HTMLResponse)
def builder_page(scenario_id: int, request: Request, db: Session = Depends(get_db)):
    return ui_scenario.scenario_builder_page(scenario_id, request, db)


@router.get("/{scenario_id}/screens/{index}", response_class=HTMLResponse)
def scenario_screen_partial(
    scenario_id: int, index: int, request: Request, db: Session = Depends(get_db)
):
    return ui_scenario.scenario_screen_partial(scenario_id, index, request, db)


@router.get("/{scenario_id}/builder/screens/{index}", response_class=HTMLResponse)
def scenario_builder_screen_partial(
    scenario_id: int, index: int, request: Request, db: Session = Depends(get_db)
):
    return ui_scenario.builder_screen_partial(scenario_id, index, request, db)
