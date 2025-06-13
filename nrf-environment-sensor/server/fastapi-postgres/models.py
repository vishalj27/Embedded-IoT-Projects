from sqlalchemy import Boolean, Column, Integer, Float, DateTime, String
from sqlalchemy.orm import relationship
from database import Base

# Define the table schema
class SensorDataTable(Base):
    __tablename__ = 'sensor_data'

    id = Column(Integer, primary_key=True, index=True)
    deviceid = Column(Integer, index=True)
    temperature = Column(Float, nullable=True)
    humidity = Column(Float, nullable=True)
    timestamp = Column(DateTime, index=True)