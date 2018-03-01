FROM azureiotedge/azureiotedge-module-base:1.0.0-preview001-linux-arm32v7

USER moduleuser

ARG EXE_DIR=.

WORKDIR /app

COPY $EXE_DIR/ ./

CMD ["dotnet", "azureiotedge-simulated-temperature-sensor.dll"]