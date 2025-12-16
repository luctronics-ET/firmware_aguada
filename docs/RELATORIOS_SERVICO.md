# Sistema de Relatórios de Serviço e Balanço Hídrico

## Visão Geral

Sistema completo para geração de relatórios operacionais com cálculos automáticos de consumo, abastecimento e balanço hídrico. Integra dados dos sensores ESP32-C3 com informações manuais preenchidas pelos operadores.

---

## Estrutura de Dados

### 1. Tabelas Principais

#### `relatorios_servico`
Relatório principal de cada turno operacional.

**Campos:**
- `data_relatorio`: Data do relatório (DATE)
- `turno`: MANHA, TARDE, NOITE, 24H
- `operador`: Nome do operador responsável
- `supervisor`: Nome do supervisor (opcional)
- `status_geral`: NORMAL, ALERTA, EMERGENCIA, MANUTENCAO
- `condicoes_climaticas`: Descrição das condições climáticas
- `ocorrencias`: Eventos importantes do turno
- `manutencoes_realizadas`: Serviços executados
- `pendencias`: Tarefas pendentes
- `dados_sensores_json`: Snapshot dos sensores (JSON)
- `validado`: Flag de validação (BOOLEAN)
- `validado_por`: Nome do supervisor validador
- `validado_em`: Timestamp da validação

#### `relatorio_reservatorios`
Detalhamento por reservatório em cada relatório.

**Campos automáticos (preenchidos por sensores):**
- `nivel_inicial_cm`, `nivel_final_cm`
- `percentual_inicial`, `percentual_final`
- `volume_inicial_litros`, `volume_final_litros`
- `dados_automaticos`: TRUE se veio de sensor

**Campos manuais (preenchidos por operador):**
- `abastecimento_litros`: Volume bombeado recebido
- `horario_abastecimento`: Ex: "08:30-09:45"
- `bomba_utilizada`: BOR_CB3_MD1, BOR_CB3_ME1, etc
- `valvula_entrada`, `valvula_saida`: ABERTA, FECHADA, PARCIAL
- `estado_operacional`: NORMAL, ALERTA, CRITICO, MANUTENCAO
- `observacoes`: Notas específicas do reservatório

**Campos calculados:**
- `consumo_litros`: `volume_inicial - volume_final`
- `consumo_m3`: `consumo_litros / 1000`

#### `eventos_abastecimento`
Registro de cada operação de bombeamento entre reservatórios.

**Campos:**
- `datetime`: Momento do evento
- `reservatorio_origem`: Ex: RCB3
- `reservatorio_destino`: Ex: RCON
- `volume_litros`: Volume transferido
- `duracao_minutos`: Duração do bombeamento
- `bomba_utilizada`: BOR_CB3_MD1, BOR_CB3_ME1
- `vazao_lpm`: Vazão média calculada (L/min)
- `operador`: Nome do operador
- `observacoes`: Notas adicionais

#### `balanco_hidrico`
Balanço hídrico calculado por período.

**Volumes:**
- `volume_inicial_litros`, `volume_final_litros`: Medidos pelos sensores
- `entrada_total_litros`: Total abastecido (soma de eventos_abastecimento)
- `saida_total_litros`: Total consumido (calculado)

**Cálculos:**
- `variacao_litros`: `volume_final - volume_inicial` (medição real)
- `balanco_litros`: `entrada - saida` (cálculo teórico)
- `divergencia_litros`: `variacao - balanco` (perdas, vazamentos)
- `percentual_divergencia`: `(divergencia / entrada) * 100`

**Fórmulas:**
```
consumo = volume_inicial + entrada - volume_final
balanco = entrada - consumo
divergencia = variacao_real - balanco_calculado
```

Se `divergencia > 0`: Possível vazamento ou perdas não contabilizadas  
Se `divergencia < 0`: Possível entrada não registrada

---

## API de Relatórios

### Endpoint: `/api/relatorios.php`

#### 1. Listar Relatórios
```http
GET /api/relatorios.php?action=list&page=1&limit=20
```

**Filtros opcionais:**
- `data_inicio`: YYYY-MM-DD
- `data_fim`: YYYY-MM-DD
- `operador`: Nome (busca parcial)
- `validado`: true/false

**Resposta:**
```json
{
  "success": true,
  "data": [
    {
      "id": 1,
      "data_relatorio": "2025-12-14",
      "turno": "MANHA",
      "operador": "João Silva",
      "status_geral": "NORMAL",
      "validado": false,
      "num_reservatorios": 5,
      "consumo_total": 12000,
      "abastecimento_total": 15000
    }
  ],
  "pagination": {
    "page": 1,
    "limit": 20,
    "total": 45,
    "pages": 3
  }
}
```

#### 2. Criar Relatório
```http
POST /api/relatorios.php?action=create
Content-Type: application/json

{
  "data_relatorio": "2025-12-15",
  "turno": "MANHA",
  "operador": "João Silva",
  "supervisor": "Carlos Oliveira",
  "status_geral": "NORMAL",
  "condicoes_climaticas": "Sol, 25°C",
  "ocorrencias": "Operação normal",
  "reservatorios": [
    {
      "reservatorio_id": "RCON",
      "nivel_inicial_cm": 154,
      "nivel_final_cm": 120,
      "percentual_inicial": 70,
      "percentual_final": 55,
      "volume_inicial_litros": 56000,
      "volume_final_litros": 44000,
      "consumo_litros": 12000,
      "abastecimento_litros": 0,
      "valvula_entrada": "FECHADA",
      "valvula_saida": "ABERTA",
      "estado_operacional": "NORMAL",
      "dados_automaticos": true
    }
  ]
}
```

**Resposta:**
```json
{
  "success": true,
  "relatorio_id": 15,
  "message": "Relatório criado com sucesso"
}
```

#### 3. Obter Relatório Específico
```http
GET /api/relatorios.php?action=get&id=15
```

#### 4. Validar Relatório (Supervisor)
```http
POST /api/relatorios.php?action=validate
Content-Type: application/json

{
  "id": 15,
  "validado_por": "Carlos Oliveira"
}
```

#### 5. Registrar Abastecimento
```http
POST /api/relatorios.php?action=registrar_abastecimento
Content-Type: application/json

{
  "datetime": "2025-12-15 08:30:00",
  "reservatorio_origem": "RCB3",
  "reservatorio_destino": "RCON",
  "volume_litros": 15000,
  "duracao_minutos": 25,
  "bomba_utilizada": "BOR_CB3_ME1",
  "operador": "João Silva",
  "observacoes": "Bombeamento normal"
}
```

**Vazão calculada automaticamente:**
```
vazao_lpm = volume_litros / duracao_minutos
vazao_lpm = 15000 / 25 = 600 L/min
```

#### 6. Calcular Balanço Hídrico
```http
POST /api/relatorios.php?action=calcular_balanco
Content-Type: application/json

{
  "reservatorio": "RCON",
  "periodo_inicio": "2025-12-15 06:00:00",
  "periodo_fim": "2025-12-15 14:00:00"
}
```

**Resposta:**
```json
{
  "success": true,
  "data": {
    "reservatorio_id": "RCON",
    "periodo_inicio": "2025-12-15 06:00:00",
    "periodo_fim": "2025-12-15 14:00:00",
    "volume_inicial_litros": 56000,
    "volume_final_litros": 44000,
    "entrada_total_litros": 15000,
    "entrada_eventos": 1,
    "saida_total_litros": 27000,
    "variacao_litros": -12000,
    "balanco_litros": -12000,
    "divergencia_litros": 0,
    "percentual_divergencia": 0.00
  }
}
```

**Interpretação:**
- Volume caiu 12.000 L (variação negativa)
- Recebeu 15.000 L de abastecimento
- Consumiu 27.000 L (15.000 entrada - 12.000 variação)
- Divergência zero = balanço fechado corretamente

#### 7. Obter Balanço Diário
```http
GET /api/relatorios.php?action=get_balanco_diario
    &data_inicio=2025-12-08
    &data_fim=2025-12-15
    &reservatorio=RCON
```

#### 8. Listar Relatórios Pendentes
```http
GET /api/relatorios.php?action=get_pendentes
```

Retorna relatórios não validados dos últimos 7 dias.

---

## Interface Web

### `relatorio_servico.html`

Interface completa com:

#### 1. Formulário de Identificação
- Data do relatório (padrão: hoje)
- Turno (Manhã, Tarde, Noite, 24H)
- Operador e Supervisor
- Status geral do sistema
- Condições climáticas

#### 2. Tabela de Reservatórios
Colunas:
- **Reservatório**: Nome + badge "SENSOR" se tiver sensor
- **Níveis**: Inicial e Final (cm)
- **Percentuais**: Inicial e Final (0-100%)
- **Volumes**: Inicial e Final (litros)
- **Consumo**: Calculado automaticamente
- **Abastecimento**: Preenchido manualmente
- **Bomba**: Dropdown (ME1 Elétrica, MD1 Diesel)
- **Válvulas**: Entrada e Saída (Aberta, Fechada, Parcial)
- **Estado**: Normal, Alerta, Crítico, Manutenção
- **Observações**: Campo texto livre

#### 3. Preenchimento Automático
Campos com fundo azul claro são **preenchidos automaticamente** ao carregar a página:
- Nível inicial (última leitura do sensor)
- Percentual inicial
- Volume inicial

**Cálculo automático de consumo:**
```javascript
consumo = volume_inicial - volume_final
```

Atualizado em tempo real quando operador preenche volume final.

#### 4. Cards de Balanço Hídrico
Três cards coloridos mostram totais consolidados:
- **Consumo Total**: Soma de todos os reservatórios (roxo)
- **Abastecimento Total**: Soma de entradas (azul)
- **Variação Líquida**: Diferença (verde)

Valores em litros e m³.

#### 5. Observações
- **Ocorrências do Turno**: Eventos importantes
- **Manutenções Realizadas**: Serviços executados
- **Pendências**: Tarefas para próximos turnos

#### 6. Botões de Ação
- **🖨️ Imprimir**: Abre diálogo de impressão
- **💾 Salvar Rascunho**: Salva no localStorage (não envia ao servidor)
- **✓ Finalizar Relatório**: Envia ao backend (POST /api/relatorios.php)
- **✓ Validar (Supervisor)**: Só aparece para supervisores

---

## Stored Procedure: `calcular_balanco_hidrico`

### Lógica de Cálculo

```sql
CALL calcular_balanco_hidrico('RCON', '2025-12-15 06:00:00', '2025-12-15 14:00:00');
```

**Passos:**

1. **Buscar volume inicial** (primeira leitura do período):
```sql
SELECT volume_l FROM leituras_v2 l
INNER JOIN sensores s ON l.node_id = s.node_id
WHERE s.alias = 'RCON' AND l.created_at >= '2025-12-15 06:00:00'
ORDER BY l.created_at ASC LIMIT 1;
```

2. **Buscar volume final** (última leitura):
```sql
SELECT volume_l FROM leituras_v2 l
INNER JOIN sensores s ON l.node_id = s.node_id
WHERE s.alias = 'RCON' AND l.created_at <= '2025-12-15 14:00:00'
ORDER BY l.created_at DESC LIMIT 1;
```

3. **Somar entradas** (abastecimentos recebidos):
```sql
SELECT SUM(volume_litros), COUNT(*) 
FROM eventos_abastecimento
WHERE reservatorio_destino = 'RCON' 
AND datetime BETWEEN '2025-12-15 06:00:00' AND '2025-12-15 14:00:00';
```

4. **Calcular variação real**:
```
variacao = volume_final - volume_inicial
```

5. **Calcular saída (consumo)**:
```
saida = volume_inicial + entrada - volume_final
```

6. **Calcular balanço**:
```
balanco = entrada - saida
```

7. **Calcular divergência**:
```
divergencia = variacao - balanco
percentual_divergencia = (divergencia / entrada) * 100
```

8. **Inserir/atualizar** tabela `balanco_hidrico` (ON DUPLICATE KEY UPDATE)

---

## Views SQL

### `vw_balanco_diario`
Consolidação diária por reservatório:
```sql
SELECT 
    DATE(periodo_inicio) as data,
    reservatorio_id,
    SUM(entrada_total_litros) as entrada_dia_litros,
    SUM(saida_total_litros) as saida_dia_litros,
    SUM(variacao_litros) as variacao_dia_litros,
    AVG(percentual_divergencia) as divergencia_media_pct
FROM balanco_hidrico
GROUP BY DATE(periodo_inicio), reservatorio_id;
```

### `vw_relatorios_pendentes`
Relatórios aguardando validação (últimos 7 dias):
```sql
SELECT 
    r.id, r.data_relatorio, r.turno, r.operador, r.status_geral,
    COUNT(rr.id) as num_reservatorios,
    DATEDIFF(CURDATE(), r.data_relatorio) as dias_atraso
FROM relatorios_servico r
LEFT JOIN relatorio_reservatorios rr ON r.id = rr.relatorio_id
WHERE r.validado = FALSE AND dias_atraso <= 7
GROUP BY r.id;
```

---

## Workflow Operacional

### Turno da Manhã (06:00 - 14:00)

1. **Início do turno (06:00)**
   - Operador acessa `relatorio_servico.html`
   - Sistema carrega dados atuais dos sensores
   - Campos de nível/volume inicial são **preenchidos automaticamente**
   - Operador verifica válvulas e equipamentos

2. **Durante o turno**
   - Se houver bombeamento:
     - Anotar horário início/fim
     - Calcular volume transferido (se não houver medidor)
     - Registrar via `/api/relatorios.php?action=registrar_abastecimento`
   
   - Se houver ocorrências:
     - Anotar em "Ocorrências do Turno"
   
   - Se houver manutenções:
     - Descrever em "Manutenções Realizadas"

3. **Fim do turno (14:00)**
   - Operador atualiza página (F5) para obter leituras finais
   - Campos de nível/volume final são **preenchidos automaticamente**
   - Sistema calcula consumo automaticamente
   - Operador preenche abastecimentos manualmente
   - Verifica balanço nos cards coloridos
   - Preenche observações
   - Clica em **"✓ Finalizar Relatório"**

4. **Validação do Supervisor**
   - Supervisor acessa lista de relatórios pendentes
   - Revisa dados e observações
   - Clica em **"✓ Validar (Supervisor)"**
   - Relatório validado não pode mais ser editado

---

## Exemplos de Uso

### Exemplo 1: Relatório Normal (Consumo Sem Abastecimento)

**Situação:** RCON consumindo água durante a noite (sem bombeamento)

**Dados:**
- Turno: NOITE (22:00 - 06:00)
- Volume inicial: 60.000 L (75%)
- Volume final: 48.000 L (60%)
- Abastecimento: 0 L

**Cálculo:**
```
consumo = 60000 - 48000 = 12000 L (12 m³)
entrada = 0 L
variacao = -12000 L
balanco = 0 - 12000 = -12000 L
divergencia = -12000 - (-12000) = 0 L ✓
```

**Interpretação:** Consumo de 12 m³ durante a noite. Balanço fechado.

### Exemplo 2: Relatório Com Abastecimento

**Situação:** RCON recebe 15 m³ da CB3 e continua consumindo

**Dados:**
- Turno: MANHA (06:00 - 14:00)
- Volume inicial: 44.000 L (55%)
- Volume final: 50.000 L (62%)
- Abastecimento: 15.000 L (08:30 - 09:00, bomba ME1)

**Cálculo:**
```
variacao_real = 50000 - 44000 = 6000 L
entrada = 15000 L
consumo = 44000 + 15000 - 50000 = 9000 L
balanco = 15000 - 9000 = 6000 L
divergencia = 6000 - 6000 = 0 L ✓
```

**Interpretação:** Recebeu 15 m³, consumiu 9 m³, subiu 6 m³. Balanço fechado.

### Exemplo 3: Relatório Com Vazamento Detectado

**Situação:** Possível vazamento em RCAV

**Dados:**
- Turno: TARDE (14:00 - 22:00)
- Volume inicial: 70.000 L (87%)
- Volume final: 50.000 L (62%)
- Abastecimento: 5.000 L (16:00, bomba ME1)

**Cálculo:**
```
variacao_real = 50000 - 70000 = -20000 L
entrada = 5000 L
consumo_esperado = 70000 + 5000 - 50000 = 25000 L
balanco = 5000 - 25000 = -20000 L
divergencia = -20000 - (-20000) = 0 L

Mas se consumo típico é ~10000 L/turno:
divergencia_real = 25000 - 10000 = 15000 L (perdas)
percentual = (15000 / 5000) * 100 = 300% ⚠️
```

**Interpretação:** Consumo anormalmente alto (25 m³ vs 10 m³ típico). Possível vazamento de 15 m³. Requer investigação.

---

## Integração com Sensores

### Fluxo de Dados

```
ESP32-C3 Sensor Node
  ↓ (ESP-NOW)
Gateway ESP32
  ↓ (HTTP POST)
Backend PHP (ingest_sensorpacket.php)
  ↓ (INSERT)
MySQL leituras_v2
  ↓ (SELECT)
API relatorios.php
  ↓ (JSON)
Interface relatorio_servico.html
  ↓ (Preenchimento automático)
```

### Código JavaScript (Preenchimento Automático)

```javascript
async function carregarDadosSensores() {
    const response = await fetch('http://localhost:8080/api/scada_data.php?action=get_all');
    const data = await response.json();
    
    data.elementos.forEach(elemento => {
        if (elemento.sensor_node_id) {
            // Buscar última leitura
            const ultimaLeitura = buscarUltimaLeitura(elemento.sensor_node_id);
            
            // Preencher campos
            document.getElementById(`nivel_ini_${elemento.alias}`).value = ultimaLeitura.level_cm;
            document.getElementById(`pct_ini_${elemento.alias}`).value = ultimaLeitura.percentual;
            document.getElementById(`vol_ini_${elemento.alias}`).value = ultimaLeitura.volume_l;
            
            // Marcar como automático (fundo azul)
            document.getElementById(`vol_ini_${elemento.alias}`).classList.add('auto-fill');
        }
    });
}
```

---

## Manutenção e Troubleshooting

### Recalcular Balanços Antigos

Se houver correção em eventos_abastecimento ou leituras_v2:

```sql
-- Recalcular balanço de um período específico
CALL calcular_balanco_hidrico('RCON', '2025-12-14 06:00:00', '2025-12-14 14:00:00');

-- Recalcular todos os balanços de um dia
SELECT DISTINCT reservatorio_id FROM balanco_hidrico 
WHERE DATE(periodo_inicio) = '2025-12-14';
-- Para cada reservatorio_id, chamar CALL calcular_balanco_hidrico(...)
```

### Corrigir Relatório Validado (Exceção)

Normalmente, relatórios validados não podem ser editados. Em caso excepcional:

```sql
-- Desvalidar temporariamente
UPDATE relatorios_servico SET validado = 0 WHERE id = 15;

-- Editar via interface ou UPDATE manual

-- Revalidar
UPDATE relatorios_servico SET validado = 1, validado_em = NOW() WHERE id = 15;
```

### Backup de Relatórios

```bash
# Exportar relatórios de um mês
mysqldump -u aguada_user sensores_db \
  relatorios_servico relatorio_reservatorios eventos_abastecimento \
  --where="data_relatorio BETWEEN '2025-12-01' AND '2025-12-31'" \
  > backup_relatorios_dez2025.sql
```

---

## Próximas Melhorias

- [ ] Dashboard de relatórios (lista, filtros, busca)
- [ ] Exportação para PDF (impressão profissional)
- [ ] Gráficos de consumo diário/semanal/mensal
- [ ] Alertas automáticos (divergência > 10%)
- [ ] Comparação entre turnos/períodos
- [ ] Relatório mensal consolidado
- [ ] Assinatura digital (supervisor)
- [ ] Fotos anexadas (manutenções, anomalias)
- [ ] Notificações push (relatórios pendentes)
- [ ] App mobile para preenchimento em campo

---

**Documentação gerada em:** 2025-12-15  
**Versão:** 1.0  
**Autor:** Sistema AGUADA
