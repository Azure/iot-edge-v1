FROM microsoft/dotnet:2.0.5-runtime

ENV MODULE_NAME "AnalyzerModule.dll"

ARG EXE_DIR=.

WORKDIR /app

COPY $EXE_DIR/ ./

CMD ["./start.sh"]