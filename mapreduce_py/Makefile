VENV := venv
PYTHON := $(VENV)/bin/python
PIP := $(VENV)/bin/pip
REQUIREMENTS := requirements.txt

# Targets
.PHONY: all clean venv install build test run

all: venv install build

clean:
	rm -rf $(VENV)
	find . -name "*.pyc" -delete
	find . -name "__pycache__" -delete
	
# rm -rf protos/*.py

venv:
	python3 -m venv $(VENV)
	$(PIP) install --upgrade pip

install: venv
	$(PIP) install -r $(REQUIREMENTS)

build: install
	$(PYTHON) -m grpc_tools.protoc -I. --python_out=./mapreduce/protos --grpc_python_out=./mapreduce/protos mapreduce/protos/mapreduce.proto

# test:
# 	$(PYTHON) -m unittest discover tests/

# run:
# 	$(PYTHON) mapreduce/master.py
