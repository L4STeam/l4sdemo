#!/bin/bash

SERVER=$SERVER_A
CLIENT=$CLIENT_A
ssh ${SERVER} 'killall scp iperf'

