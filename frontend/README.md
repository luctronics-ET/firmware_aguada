# Frontend - Aguada Telemetry

Dashboard web para visualização de telemetria em tempo real.

## Status

🚧 **Em desenvolvimento** - Estrutura preparada para frontend moderno.

## Tecnologias Sugeridas

### Opção 1: React + Vite (Recomendado)
```bash
npm create vite@latest . -- --template react
npm install
npm run dev
```

**Bibliotecas úteis:**
- `recharts` ou `chart.js` - Gráficos de nível/volume
- `axios` - Requisições HTTP para backend
- `react-query` - Cache e sincronização de dados
- `socket.io-client` - Dados em tempo real (se backend suportar)

### Opção 2: Vue 3 + Vite
```bash
npm create vite@latest . -- --template vue
npm install
npm run dev
```

### Opção 3: Next.js (SSR)
```bash
npx create-next-app@latest .
npm run dev
```

### Opção 4: HTML/CSS/JS Puro
Use o template TailAdmin já disponível em:
`~/Área de trabalho/dev/REF_templates/tailadmin-free-tailwind-dashboard-template-main/`

## Features Planejadas

### Dashboard Principal
- [ ] Cards de status para cada nó (1-5)
- [ ] Gráficos de nível em tempo real
- [ ] Histórico de 24h/7d/30d
- [ ] Indicadores de bateria (vin_mv)
- [ ] Qualidade de sinal (RSSI)
- [ ] Alertas visuais (níveis críticos)

### Visualizações
- [ ] Gráfico de linha: nível ao longo do tempo
- [ ] Gráfico de barras: volume por reservatório
- [ ] Mapa de calor: qualidade de sinal
- [ ] Tabela: últimas leituras

### Funcionalidades
- [ ] Filtros por nó, data, período
- [ ] Export de dados (CSV/JSON)
- [ ] Configurações de alertas
- [ ] Modo escuro/claro
- [ ] Responsivo (mobile-first)

## Estrutura Proposta

```
frontend/
├── public/
│   └── favicon.ico
├── src/
│   ├── components/
│   │   ├── NodeCard.jsx
│   │   ├── LevelChart.jsx
│   │   ├── SignalIndicator.jsx
│   │   └── AlertBanner.jsx
│   ├── services/
│   │   └── api.js           # Cliente HTTP para backend
│   ├── hooks/
│   │   └── useTelemetry.js  # Custom hook para dados
│   ├── pages/
│   │   ├── Dashboard.jsx
│   │   ├── History.jsx
│   │   └── Settings.jsx
│   ├── App.jsx
│   └── main.jsx
├── package.json
└── vite.config.js
```

## Integração com Backend

### Configurar API endpoint
```javascript
// src/services/api.js
const API_BASE_URL = 'http://192.168.0.117:8080';

export const fetchLatestReadings = async () => {
  const response = await fetch(`${API_BASE_URL}/api/latest`);
  return response.json();
};

export const fetchNodeHistory = async (nodeId, hours = 24) => {
  const response = await fetch(`${API_BASE_URL}/api/history/${nodeId}?hours=${hours}`);
  return response.json();
};
```

### Polling de dados
```javascript
// src/hooks/useTelemetry.js
import { useEffect, useState } from 'react';
import { fetchLatestReadings } from '../services/api';

export const useTelemetry = (intervalMs = 30000) => {
  const [data, setData] = useState([]);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    const loadData = async () => {
      const readings = await fetchLatestReadings();
      setData(readings);
      setLoading(false);
    };

    loadData();
    const interval = setInterval(loadData, intervalMs);
    return () => clearInterval(interval);
  }, [intervalMs]);

  return { data, loading };
};
```

## Próximos Passos

1. **Escolher framework** (React/Vue/Next.js ou usar TailAdmin)
2. **Criar backend API** (REST endpoints estruturados)
3. **Implementar dashboard** básico
4. **Adicionar gráficos** de histórico
5. **Sistema de alertas** visuais
6. **Deploy** (Vercel/Netlify/Docker)
