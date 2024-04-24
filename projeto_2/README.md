# Projeto 1

A seguir será descrito como compilar e utilizar o sistema desenvolvido no projeto 1

## Compilação

Para compilar basta digitar o comando  `make` dentro da pasta do projeto (onde se encontra o `Makefile`)

## Execução

### Servidor

Para rodar o servidor execute `.\server.out <file>.csv` sendo <file> o nome do arquivo CSV responsável por armazenar as músicas.

### Cliente

Para rodar o cliente execute `.\client.out <IP do servidor>`. Para rodar como administrador (podendo adicionar ou remover músicas) adicione a flag `-adm` a essa linha de comando.
#### Operações
- ##### Add
  Para adicionar novas músicas, serão pedidos os metadados da música que se deseja adicionar e o ID será decidido pelo servidor
- ##### Rem
  Para remover uma música, será pedido o ID.  
- ##### List
  Para listar músicas. Em caso de filtro, depois da operção deve-se colocá-los no formato `campo=valor` separados por ";".
  Exemplo:
  ```
  list

  id=5;lang=English;
  ```
  Os parâmetros são  'id' para  ID, 'year' para Ano de Lançamento, 'title' para o nome da música, 'interpreter' para o intérprete, 'lang' para o idioma, 'type' para o tipo de música e  'chorus' para o refrão.
