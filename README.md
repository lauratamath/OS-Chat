# OS-Chat

## Libraries
- sudo apt-get install libjson-c-dev

## Compile
- gcc -o client client.c -lpthread -ljson-c
- gcc -o server server.c -lpthread -ljson-c

## Checklist:

- [x] Conexión entre cliente-servidor [puerto 888]
- [x] Conexión entre dos o más personas
- [ ] Nombre del usuario 
- [ ] No permite que el nombre del usuario se repita
- [ ] Aviso de cerrar sesión
- [ ] Chat privado
- [ ] Listado de usuarios conectados
- [ ] Mostrar status del usuario
- [ ] Desplegar información de un usuario en particula
- [ ] Ayuda
- [ ] Salir
