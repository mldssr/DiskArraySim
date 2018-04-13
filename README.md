# DiskArraySim
A light disk array simulator for DAES.

## Generate requests
1. Check the `REQ` part of `conf.conf` which defines the way to generate requests.
2. Type `make test_req`, the requests will be put into `req.csv` in a overwriting way.

## Run the simulator
1. Check the configure file `conf.conf` which defines behaviour of this simulator.
2. Type `make sim` to run the simulator. It reads requests from `req.csv` and handles them according to the configure file.
