# Sistema SCADA Hidráulico - Documentação

## Visão Geral

Sistema completo de **Supervisão, Controle e Aquisição de Dados (SCADA)** para monitoramento e controle de sistemas hidráulicos complexos.

---

## Modelo de Dados

### Hierarquia

```
Locais (Castelo de Consumo, Casa de Bombas...)
  └── Elementos (Reservatórios, Bombas, Válvulas...)
      ├── Sensores (Medições automáticas)
      ├── Estados (Posições de válvulas, status de bombas)
      └── Conexões (Tubulações entre elementos)
```

---

## Tipos de Elementos

### 1. **Armazenamento**
- **Reservatório Elevado**: Castelo d'água para distribuição por gravidade
- **Reservatório**: Tanque ao nível do solo
- **Cisterna**: Reservatório subterrâneo para captação

**Atributos:**
- Capacidade (L)
- Altura (m)
- Diâmetro (cm)
- Nível atual (via sensor ultrassônico)

### 2. **Transporte**
- **Encanamento**: Tubulação para condução de água

**Atributos:**
- Comprimento (m)
- Diâmetro (polegadas)
- Material (PVC, ferro galvanizado, cobre, PEX)
- Pressão (bar)

### 3. **Controle**
- **Válvula**: Controle de fluxo (1 entrada → 1 saída)
- **Válvula Y**: Divisor (1 entrada → 2 saídas)
- **Válvula 3 Vias**: Misturador/desviador
- **Registro**: Válvula de bloqueio
- **Boia de Nível**: Controle automático por nível

**Estados:**
- `aberta` / `fechada` / `parcial` (0-100%)
- Modo: `manual` / `automatico`

### 4. **Pressão**
- **Bomba de Pressão**: Aumenta pressão/vazão
- **Bomba de Recalque**: Eleva água para reservatórios

**Estados:**
- `ligada` / `desligada` / `falha`
- Frequência (Hz)
- Corrente (A)
- Pressão saída (bar)
- Vazão (L/h)
- Temperatura motor (°C)

### 5. **Medição**
- **Hidrômetro**: Medidor de volume consumido

**Dados:**
- Volume acumulado (m³)
- Vazão instantânea (L/h)
- Pulsos por litro

---

## Conexões

Elementos conectados formam o **grafo do sistema hidráulico**:

```
Cisterna (saída 1) 
    ↓ [Tubo 2" PVC]
Bomba Recalque (entrada 1 → saída 1)
    ↓ [Tubo 2" PVC]
Válvula Principal (entrada 1 → saída 1)
    ↓ [Tubo 1.5" PVC]
Reservatório Elevado (entrada 1)
```

**Atributos de Conexão:**
- Elemento origem/destino
- Porta origem/destino (para elementos com múltiplas entradas/saídas)
- Tipo de conexão (rosca, flange, soldada)
- Diâmetro, comprimento, material
- Estado ativo/inativo

---

## Anomalias Detectadas

Sistema detecta automaticamente:

### 1. **Vazamento**
- Volume saindo > Volume entrando (por período)
- Queda de pressão anormal
- Nível caindo mais rápido que consumo esperado

### 2. **Entupimento**
- Pressão aumentando além do normal
- Vazão reduzida com bomba ligada
- Tempo de enchimento aumentado

### 3. **Queda de Pressão**
- Pressão abaixo do limiar mínimo
- Bomba ligada mas pressão baixa (possível cavitação)

### 4. **Nível Crítico**
- Reservatório < 10% (crítico)
- Reservatório > 95% (transbordamento iminente)

### 5. **Bomba em Falha**
- Corrente acima do nominal
- Temperatura elevada
- Desligamento inesperado

### 6. **Sensor Offline**
- Sem leituras há > 5 minutos
- Valores impossíveis (outliers)

**Severidades:**
- `info`: Informativo (bomba desligada manualmente)
- `aviso`: Atenção necessária (nível baixo)
- `critico`: Ação imediata (vazamento detectado)

---

## Interface SCADA

### Tela Principal (`backend/scada.html`)

**Layout:**
```
┌─────────────────────────────────────────────────┐
│  Header: Logo, Atualizar, Configurar, Novo      │
├───────────┬──────────────────────┬──────────────┤
│  Sidebar  │  Canvas SCADA        │  Painel      │
│  Esquerda │  (Diagrama Interativo│  Direito     │
│           │   com Pan & Zoom)    │              │
│  • Locais │                       │  • Alertas   │
│  • Filtros│  [Elementos]         │  • Detalhes  │
│           │  [Conexões]          │  • Controles │
│           │  [Fluxos]            │  • Histórico │
└───────────┴──────────────────────┴──────────────┘
│  Status Bar: Sensores, Anomalias, Última update │
└─────────────────────────────────────────────────┘
```

**Funcionalidades:**

1. **Visualização Interativa:**
   - Elementos representados graficamente
   - Cores indicam estado (verde=OK, amarelo=aviso, vermelho=crítico)
   - Conexões mostram fluxo de água (setas)
   - Pan com mouse drag
   - Zoom com scroll (TODO)

2. **Controle em Tempo Real:**
   - Clicar em válvula → Abrir/Fechar
   - Clicar em bomba → Ligar/Desligar
   - Registro de ações (quem, quando, porquê)

3. **Monitoramento:**
   - Níveis de reservatórios (%)
   - Estados de válvulas (aberta/fechada)
   - Status de bombas (ligada/desligada)
   - Pressões, vazões, correntes

4. **Alertas:**
   - Notificações de anomalias
   - Severidade com cores
   - Tempo decorrido desde detecção
   - Filtro por local/tipo

---

## API REST

### Endpoints (`backend/api/scada_data.php`)

#### 1. GET `?action=get_all`
Retorna todos os dados do sistema

**Response:**
```json
{
  "locais": [...],
  "elementos": [...],
  "conexoes": [...],
  "anomalias": [...],
  "tipos_elemento": [...]
}
```

#### 2. POST `?action=toggle_valvula`
Altera estado de uma válvula

**Body:**
```json
{
  "elemento_id": 5,
  "estado": "aberta",  // ou "fechada", "parcial"
  "usuario_id": 1,
  "motivo": "Manutenção programada"
}
```

**Response:**
```json
{
  "success": true,
  "message": "Válvula alterada"
}
```

#### 3. POST `?action=toggle_bomba`
Liga/desliga bomba

**Body:**
```json
{
  "elemento_id": 4,
  "estado": "ligada",  // ou "desligada"
  "usuario_id": 1,
  "motivo": "Operação normal"
}
```

#### 4. GET `?action=get_history&elemento_id=1&hours=24`
Histórico de leituras e estados de um elemento

**Response:**
```json
{
  "leituras": [
    {
      "timestamp": "2025-12-14 16:30:00",
      "distance_cm": 110,
      "level_cm": 360,
      "percentual": 80,
      "volume_l": 64000
    }
  ],
  "estados_valvula": [...],
  "estados_bomba": [...]
}
```

---

## Aplicação dos Dados

### Migração de Dados Existentes

```sql
-- 1. Criar locais
INSERT INTO locais (nome, descricao) VALUES
('Castelo de Consumo', 'Reservatório elevado principal'),
('Casa de Bombas', 'Sala de máquinas');

-- 2. Criar elementos
INSERT INTO elementos (nome, tipo_id, local_id, capacidade_l, altura_m, diametro_cm) VALUES
('Reservatório Consumo', 
 (SELECT id FROM tipos_elemento WHERE codigo = 'reservatorio_elevado'),
 (SELECT id FROM locais WHERE nome = 'Castelo de Consumo'),
 80000, 450, 200);

-- 3. Associar sensores existentes
INSERT INTO sensores (node_id, mac, tipo_sensor, elemento_id, posicao_sensor, offset_cm)
SELECT 
    node_id,
    mac,
    'ultrasonic',
    (SELECT id FROM elementos WHERE nome = 'Reservatório Consumo'),
    'topo',
    20
FROM (SELECT DISTINCT node_id, mac FROM leituras_v2 WHERE node_id = 1) AS unique_sensors;

-- 4. Atualizar leituras com FK para sensor
UPDATE leituras_v2 l
SET l.sensor_id = (SELECT s.id FROM sensores s WHERE s.node_id = l.node_id)
WHERE l.sensor_id IS NULL;
```

---

## Instalação

### 1. Aplicar Migration

```bash
cd /home/luciano/firmware_aguada
sudo mysql -u aguada_user sensores_db < database/migrations/003_sistema_scada.sql
```

### 2. Verificar Tabelas

```bash
sudo mysql sensores_db -e "SHOW TABLES;"
```

**Esperado:**
- `locais`
- `tipos_elemento`
- `elementos`
- `conexoes`
- `sensores`
- `estados_valvula`
- `estados_bomba`
- `anomalias`
- `leituras_v2` (já existente, com nova FK)

### 3. Acessar Interface SCADA

```
http://192.168.0.117:8080/scada.html
```

---

## Próximos Passos

1. ✅ Schema SQL criado
2. ✅ Interface SCADA básica
3. ✅ API REST funcional
4. ⏳ Aplicar migration no banco
5. ⏳ Inserir dados de exemplo (locais, elementos, conexões)
6. ⏳ Associar sensores existentes aos elementos
7. ⏳ Testar controle de válvulas/bombas via interface
8. ⏳ Implementar detecção automática de anomalias
9. ⏳ Adicionar gráficos de histórico (Chart.js)
10. ⏳ Sistema de autenticação/autorização

---

## Exemplo Completo: Sistema de um Prédio

```
📍 LOCAIS:
- Castelo de Consumo (120m altitude)
- Castelo de Incêndio (125m altitude)
- Casa de Bombas (10m altitude)
- Cisterna Subterrânea (-5m altitude)

🔧 ELEMENTOS:
- Cisterna Principal (120.000 L) [sensor ultrassônico]
- Bomba Recalque 01 (2 CV)
- Válvula Castelo Consumo (2")
- Reservatório Consumo (80.000 L) [sensor ultrassônico]
- Válvula Castelo Incêndio (1.5")
- Reservatório Incêndio (50.000 L) [sensor ultrassônico]
- Hidrômetro Entrada

🔗 CONEXÕES:
Cisterna → Bomba → Válvula → Reservatório Consumo → Distribuição
                 → Válvula → Reservatório Incêndio → Rede Incêndio

📊 MONITORAMENTO:
- Nível Cisterna: 85% (102.000 L)
- Bomba: Ligada (45 Hz, 12.3 A, 3.2 bar, 5.200 L/h)
- Válvula Consumo: Aberta 100%
- Nível Consumo: 78% (62.400 L)
- Válvula Incêndio: Aberta 20% (standby)
- Nível Incêndio: 95% (47.500 L)
- Hidrômetro: 1.234,56 m³ acumulados

⚠️ ALERTAS:
- [AVISO] Reservatório Incêndio > 95% - Verificar boia
```

---

## Referências

- **IEC 61131**: Padrão para sistemas SCADA industriais
- **ISA-88**: Batch control padrão
- **Grafana + InfluxDB**: Alternativa para visualização avançada
- **Node-RED**: Automação visual (fluxos)
- **MQTT**: Protocolo para IoT em tempo real
