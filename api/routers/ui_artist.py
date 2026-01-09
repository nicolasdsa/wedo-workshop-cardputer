from fastapi import APIRouter, Depends, Request
from fastapi.responses import HTMLResponse
from sqlalchemy.orm import Session

from controllers import ui_artist
from core.dependencies import get_db

router = APIRouter(
    prefix="/ui",
    tags=["ui-artists"],
    default_response_class=HTMLResponse,
)


@router.get("/", response_class=HTMLResponse)
def landing(request: Request):
    return ui_artist.landing_page(request)


@router.get("/artists", response_class=HTMLResponse)
def list_artists(request: Request, db: Session = Depends(get_db)):
    return ui_artist.list_artists_page(request, db)


@router.get("/artists/list", response_class=HTMLResponse)
def list_artists_partial(request: Request, db: Session = Depends(get_db)):
    return ui_artist.list_artists_partial(request, request.query_params, db)


@router.get("/artists/new-modal", response_class=HTMLResponse)
def new_artist_modal(request: Request):
    return ui_artist.new_artist_modal(request)


@router.get("/artists/{artist_id}/edit-modal", response_class=HTMLResponse)
def edit_artist_modal(
    artist_id: int, request: Request, db: Session = Depends(get_db)
):
    return ui_artist.edit_artist_modal(artist_id, request, db)


@router.get("/artists/{artist_id}/delete-modal", response_class=HTMLResponse)
def delete_artist_modal(
    artist_id: int, request: Request, db: Session = Depends(get_db)
):
    return ui_artist.delete_artist_modal(artist_id, request, db)


@router.post("/artists", response_class=HTMLResponse)
async def create_artist(request: Request, db: Session = Depends(get_db)):
    return await ui_artist.create_artist_action(request, db)


@router.post("/artists/{artist_id}", response_class=HTMLResponse)
async def update_artist(
    artist_id: int, request: Request, db: Session = Depends(get_db)
):
    return await ui_artist.update_artist_action(artist_id, request, db)


@router.post("/artists/{artist_id}/delete", response_class=HTMLResponse)
async def delete_artist(
    artist_id: int, request: Request, db: Session = Depends(get_db)
):
    return await ui_artist.delete_artist_action(artist_id, request, db)


@router.get("/artists/{artist_id}", response_class=HTMLResponse)
def artist_profile(artist_id: int, request: Request, db: Session = Depends(get_db)):
    return ui_artist.artist_profile_page(artist_id, request, db)
