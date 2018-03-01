FROM microsoft/dotnet:2.0.5-runtime

ARG EXE_DIR=.

WORKDIR /app

COPY $EXE_DIR/ ./

CMD ["dotnet", "azureiotedge-simulated-temperature-sensor.dll"]