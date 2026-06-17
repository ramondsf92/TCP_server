# Para Compilar:
gcc ts.c sockets.c -o ts (servidor)
gcc tc.c sockets.c -o tc -pthread (cliente)

# TCP server

 - salvar lista de usuarios 
 - Mensagem direta(D)
 - Mensagem broadcast(B)
 - Lista de usuarios(L)
 - Tratamento do servidor
 - logout(S)
 - Refresh da tela
 - Guardar as ultimas 5 mensagens


# Mensagens

Registro:
R999Nome|IP|Porta|

Notas:
Não há uma opção de menu para registro, pois se registra na entrada do programa
cliente:
Exemplo de chamada: ./cli NomeUsuário IPUsuário PortaUsuário IPServidor
Inclui usuário no servidor e o servidor dispara a nova lista para todos os usuários ativos.

Saída:
S999Nome|

Notas:
Exclui o usuário do servidor, encerra as threads, encerra as variáveis para lock/unlock e
o programa.

Mensagem direcionada à um usuário:
D999NomeDestinatário|Mensagem|

Mensagem em broadcast:
B999NomeRemetente|Mensagem|

Nota:
Rotina para fazer um unicast para todos os usuários, menos para o que enviou a
mensagem.

Lista de usuários:
L999Nome1|IP1|Porta1|Nome2|IP2|Porta2|... NomeN|IPN|PortaN|

Retorno de comando acerto (OK):
A000

Retorno de comando erro (Ñ OK):
N000
