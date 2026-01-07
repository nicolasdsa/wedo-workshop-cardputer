from __future__ import annotations

from fastapi import Depends, Request
from fastapi.responses import HTMLResponse
from sqlalchemy.orm import Session

from core.dependencies import get_db
from core.templates import templates
from services import artist as artist_service
from services import scenario as scenario_service


def landing_page(request: Request) -> HTMLResponse:
    return templates.TemplateResponse(
        "home.html",
        {"request": request},
    )


def list_artists_page(request: Request, db: Session = Depends(get_db)) -> HTMLResponse:
    artists = artist_service.list_artists(db)
    return templates.TemplateResponse(
        "artists/list.html",
        {
            "request": request,
            "artists": artists,
        },
    )


def artist_profile_page(
    artist_id: int, request: Request, db: Session = Depends(get_db)
) -> HTMLResponse:
    artist = artist_service.get_artist(db, artist_id)
    scenarios = scenario_service.list_scenarios_by_artist(db, artist_id)
    return templates.TemplateResponse(
        "artists/profile.html",
        {
            "request": request,
            "artist": artist,
            "scenarios": scenarios,
        },
    )
