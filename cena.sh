#!/bin/bash

FILE="cena"

FILOSOFI=21
STARV=A
SOLUZIONE=A
DEADLOCK=A

gcc -o cena cena.c -pthread

while [ $FILOSOFI -gt 20 ] 
do
	read -p "Inserisci il numero di filosofi: " FILOSOFI
	
done

while [ $STARV != "N" ] && [ $STARV != "S" ] 
do
	read -p "Vuoi abilitare il flag di controllo per la starvation? (S/N) " STARV

done

while [ $SOLUZIONE != "N" ] && [ $SOLUZIONE != "S" ] 
do
	read -p "Vuoi abilitare il flag per la risoluzione dello stallo? (S/N) " SOLUZIONE

done

while [ $DEADLOCK != "N" ] && [ $DEADLOCK != "S" ] 
do
	read -p "Vuoi abilitare il flag per il controllo dello stallo? (S/N) " DEADLOCK

done

echo "Il Programma si sta avviando..."
sleep 1

chmod +x cena

./cena $FILOSOFI $STARV $SOLUZIONE $DEADLOCK
