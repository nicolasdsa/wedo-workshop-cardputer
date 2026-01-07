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


@router.get("/artists/{artist_id}", response_class=HTMLResponse)
def artist_profile(artist_id: int, request: Request, db: Session = Depends(get_db)):
    return ui_artist.artist_profile_page(artist_id, request, db)
