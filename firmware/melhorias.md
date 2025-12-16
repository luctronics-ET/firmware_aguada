# Melhorias e Considerações para o Gateway

## Objetivos
- Receber ESP-NOW dos nós e reenviar ao servidor com mínima perda.
- Oferecer acesso local simples (USB e Wi-Fi) para configuração, debug e visualização.

## Canal e Rádio
- Padronizar canal fixo (ex.: 11) no roteador/AP e em todos os nós ESP-NOW; evitar roaming ou canais dinâmicos.
- No gateway, operar STA+ESP-NOW sem chamar `esp_wifi_set_channel` depois de conectar; usar o canal do AP fixo.
- Se AP não puder ser fixo, opção é operar somente em AP (sem internet) ou migrar para dois rádios (não trivial no ESP32 clássico).

## Fluxo de Processamento
- Callback ESP-NOW: apenas enfileirar (`xQueueSendFromISR`) e sair; nada de HTTP ou prints pesados.
- Tarefa de processamento: valida/normaliza pacote, preenche RSSI/timestamp, envia para worker HTTP/UDP.
- Worker HTTP: `esp_http_client` com timeout curto (2–3 s), fila de best-effort; em falha, opcionalmente gravar em SD para retry.
- Métricas: contadores de recebidos, parse ok, erros, fila cheia.

## Modos de Operação
- **Modo Configuração**: SoftAP canal fixo (ex.: 11), sem STA. Portal cativo básico com DNS hijack + `esp_http_server` (IDF) ou WiFiManager (Arduino/PlatformIO). Páginas: SSID/PASS, URL do servidor, teste de envio.
- **Modo Operação**: STA no AP fixo + ESP-NOW; worker HTTP ativo. Portal cativo desligado para economizar RAM/CPU. Botão curta/longa para alternar modo ou reset de fábrica (apagar NVS/JSON).
- CLI via USB CDC (TinyUSB/CDC-ACM) para configuração plug-and-play e debug textual.

## Web UI Local
- Página leve (HTML+CSS inline) servida pelo gateway em AP ou STA: status de nós (MAC, seq, RSSI, últimos valores), fila e erros, botão “reenviar últimos N”.
- Evitar frameworks; manter estático e pequeno.

## Periféricos Opcionais
- OLED I2C (SSD1306): atualizar a cada 500–1000 ms com status (IP, canal, nó recente, fila). Custo baixo se update for raramente.
- Botão: debounce; curta = modo config temporário; longa = reset fábrica.
- Buzzer: beeps curtos para boot/erro Wi-Fi.
- microSD (SPI): buffer e flush em lote para não travar rádio; útil para store-and-forward se backend cair.

## Segurança e Robustez
- Guardar credenciais/URL em NVS ou arquivo JSON; validar antes de aplicar.
- Se falhar STA, voltar a modo config automaticamente após N tentativas.
- Usar watchdog simples na tarefa de rede; monitorar fila cheia e descartar com log leve.

## Decisões por Impacto
- **Baixo custo/alto ganho**: fila no callback; worker HTTP; canal fixo no AP; botão de modo; portal cativo simples; CLI USB.
- **Médio custo**: microSD para retry; página de status local; OLED.
- **Alto custo**: HTTPS/TLS (CPU e RAM); dois rádios ou coexistência com AP dinâmico; portal cativo avançado com charts.

## Próximos Passos
1) Fixar canal do AP (ex.: 11) e alinhar nós.
2) Implementar estados Config/Operação com botão curta/longa; SoftAP + página de config.
3) Mover envio HTTP para worker com fila; medir perdas e fila cheia.
4) (Opcional) microSD para buffer de falhas e OLED de status.
