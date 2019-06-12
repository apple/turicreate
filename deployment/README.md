# Deploying Turi Create ML Models using Flask and Docker

We are assuming that you’ve already built and saved your model onto your local machine. We can deploy our model by following the steps below - 

1. Wrap your model with a restful web service using Flask. This makes it easily accessible.
2. Containerize your web service with docker. This ensures fault tolerance by allowing us to spin up new containers very quickly should one break down.

## MODELS SUPPORTED

All Turi Create models are supported except sound classification.

## API CALLS

The API calls for the Dockerized container of TuriCreate

/info 

This will show all the information about the trained model. 

/predict

It performs predict function on the model provided. The appropriate data type is returned to the user.

# Install Docker

Look at the INSTALL.md file to figure out what dependencies to install

# Build Dockerfile

Look at the BUILD.md file to figure out how to build a docker container for the api
