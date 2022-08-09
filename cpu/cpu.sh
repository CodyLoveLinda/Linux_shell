#!/bin/bash
while  true
do 
  top -bcisSn1 >>cpu.log
  sleep 10
done
