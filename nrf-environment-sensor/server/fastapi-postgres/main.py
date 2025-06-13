# fastapi_server.py
from fastapi import FastAPI, HTTPException, Depends, Request
from typing import Union, Dict, Annotated
from pydantic import BaseModel, Field
from fastapi.responses import HTMLResponse, JSONResponse
import csv
from fastapi.templating import Jinja2Templates
import os
from csv import writer
from datetime import datetime
from fastapi.staticfiles import StaticFiles
import random


import models
from database import engine, SessionLocal
from sqlalchemy.orm import Session

app = FastAPI()

models.Base.metadata.create_all(bind=engine)

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
templates = Jinja2Templates(directory=os.path.join(BASE_DIR, "static"))


class SensorData(BaseModel):
    deviceid: int
    data: dict

def get_db():
    db = SessionLocal()
    try:
        yield db
    finally:
        db.close()

db_dependancy = Annotated[Session, Depends(get_db)]

# @app.get("/")
# def read_root():
#     return {"Hello": "World"}

@app.get("/", response_class=HTMLResponse)
async def get_index(request: Request):
    return templates.TemplateResponse("index.html", {"request": request})


@app.post("/save-data-db/")
async def save_data(sensor_data: SensorData):
    db: Session = SessionLocal()
    try:
        device_id = sensor_data.deviceid
        data = sensor_data.data
        print(data)

        temperature = data['temperature'][0]['temp'] if 'temperature' in data else None
        humidity = data['humidity'][0]['percentage'] if 'humidity' in data else None
        timestamp = datetime.now()

        # Insert data into the PostgreSQL database
        db.add(models.SensorDataTable(
            deviceid=device_id,
            temperature=temperature,
            humidity=humidity,
            timestamp=timestamp
        ))
        db.commit()
        # db.refresh(models.SensorDataTable)

        return {"message": "Data saved successfully."}
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Failed to save data: {str(e)}")
    finally:
        db.close()

@app.get("/latest-data/")
def get_latest_data(db: Session = Depends(get_db)):
    try:
        # Retrieve the latest data from the database
        latest_data = db.query(models.SensorDataTable).order_by(models.SensorDataTable.timestamp.desc()).first()
        if not latest_data:
            raise HTTPException(status_code=404, detail="No data found")

        return {
            "deviceid": latest_data.deviceid,
            "temperature": latest_data.temperature,
            "humidity": latest_data.humidity,
        }
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Failed to fetch latest data: {str(e)}")


if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)