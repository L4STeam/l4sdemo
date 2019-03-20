#!/bin/bash
SERVER=$SERVER_B
CLIENT=$CLIENT_B

ssh ${SERVER} 'killall scp iperf'
