# Computer Diagnostic Center

Aplicação de diagnóstico de sistema para Windows, escrita em C++ com interface gráfica (raylib). Projeto final do módulo 00608 (Desenvolver programas em linguagem orientada a objetos), CET Especialista em Redes e Sistemas Computacionais, IEFP Tomar.

## O que faz

- **Dashboard:** uso de CPU, RAM e disco C:, número de processos, atualizados a cada segundo.
- **Sistema:** nome do computador, utilizador, modelo do CPU, arquitetura e tempo ligado.
- **Rede:** adaptador, IP, gateway, DNS, MAC e teste de Internet (ping a 1.1.1.1).
- **Segurança:** estado do Windows Defender e da Firewall.
- **Health Score (0 a 100):** calculado por regras simples sobre os dados acima.
- **Diagnóstico completo:** resultados NORMAL, WARNING ou CRITICAL para CPU, RAM, disco, rede, Defender e Firewall.
- **Relatório técnico** em `.txt` e registo de eventos em `diagnostic.log`.
- **Atalhos** para ferramentas do Windows (Gestor de Dispositivos, Serviços, Visualizador de Eventos, etc.). A aplicação não altera nada no sistema.

Quando uma informação não pode ser obtida, a aplicação mostra N/A.

## Estrutura do código

Está tudo em `main.cpp`, organizado em classes com responsabilidades separadas:

- `SystemData`: guarda o estado do sistema.
- `SystemMonitor`: mede CPU, RAM e processos.
- `SystemCollector`: recolhe informação de sistema, disco, rede e segurança.
- `HealthCalculator`: calcula o Health Score.
- `DiagnosticEngine`: executa os diagnósticos.
- `ReportManager`: gera os relatórios.
- `DiagnosticController`: coordena os objetos anteriores.

## Como compilar

Requisitos: Windows, compilador C++ (MinGW) e [raylib](https://www.raylib.com/).

O projeto foi feito com o Red Panda C++. Abre `Final_ProjectKj.dev`. Flags do linker: `-lraylib -lopengl32 -lgdi32 -lwinmm`.

O ficheiro `kj_logo.png` tem de estar na mesma pasta que o executável.

## Limitações conhecidas

- Funciona apenas em Windows.
- Todo o código está num único ficheiro. Pretendo separá-lo em ficheiros `.h` e `.cpp`.
- Os atributos de `SystemData` são públicos, a interface lê-os diretamente.
- Se o estado do Defender ou da Firewall não puder ser confirmado, aparece N/A. A aplicação não distingue "desligado" de "indisponível".
- A recolha de rede e segurança usa PowerShell e bloqueia a interface durante alguns segundos no refresh.
- A rede mostra o primeiro adaptador com IP, que pode não ser o ativo se houver máquinas virtuais ou VPN.
- O teste de Internet usa ping e pode falhar em redes que bloqueiam ICMP.

## Autor

André Ribeiro
