from fastapi import FastAPI, HTTPException,Request
from fastapi.responses import JSONResponse
from pydantic import BaseModel
from datetime import datetime
from typing import List
import struct
import os
import time
import csv



app = FastAPI()

@app.post("/debug")
async def receive_vibration_data(request: Request):
    try:
        raw_body = await request.body()
        print(raw_body)

        return {"status": "success", "message": "Raw data received successfully"}

    except Exception as e:
        print("Error:", str(e))
        raise HTTPException(status_code=500, detail="Failed to process the data")



# if __name__ == "__main__":
#     import uvicorn
#     uvicorn.run(app, host="0.0.0.0", port=8000)
