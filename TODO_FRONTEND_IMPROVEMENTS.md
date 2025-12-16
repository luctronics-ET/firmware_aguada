# 📋 TODO - Melhorias Frontend AGUADA

**Data:** 15 de dezembro de 2025  
**Baseado em:** Análise de projetos anteriores em `.REFERENCIAS/`

---

## 🎨 1. DESIGN SYSTEM & UI MODERNIZATION

### 1.1 Implementar TailAdmin Design Completo
- [ ] **Sidebar consistente** em todas as páginas
  - Referência: `.REFERENCIAS/tailadmin-*/src/partials/sidebar.html`
  - Dark mode toggle
  - Navegação: Dashboard → SCADA → Relatórios → Mapa → Configurações
  - User profile dropdown
  - Logo CMASM com ícone animado

- [ ] **Header unificado**
  - Breadcrumb navigation
  - Data/hora em tempo real
  - Status do sistema (online/offline)
  - Notificações dropdown

- [ ] **Cards modernos com glassmorphism**
  - Referência: `.REFERENCIAS/templates/aqua-flow.html` (backdrop-filter)
  - Sombras sutis e bordas gradient
  - Hover effects com elevation
  - Loading skeletons

### 1.2 Color Palette & Tema
- [ ] **Definir CSS Variables**
  ```css
  :root {
    --primary-blue: #0066cc;
    --secondary-blue: #00a8ff;
    --success-green: #00c853;
    --warning-yellow: #ffd600;
    --danger-red: #ff1744;
    --dark-bg: #1a1a2e;
    --card-bg: #16213e;
  }
  ```
- [ ] **Dark mode suport** (LocalStorage persistence)
- [ ] **Animações de transição** suaves em todos os elementos

---

## 🗺️ 2. MAPA INTERATIVO DA REDE

### 2.1 Página Mapa com Coordenadas Reais
**Referência:** `.REFERENCIAS/MapaPage.tsx`

- [ ] **Criar `mapa.html`** com layout moderno
- [ ] **Biblioteca de mapas:**
  - Opção 1: Leaflet.js (OpenStreetMap) - Leve, open-source
  - Opção 2: SVG customizado com drag/zoom
  
- [ ] **Definir coordenadas dos ativos:**
  ```javascript
  const nodes = {
    RCON: { lat: -25.XXXX, lng: -49.XXXX, name: 'Reservatório Condomínio' },
    RCAV: { lat: -25.XXXX, lng: -49.XXXX, name: 'Reservatório Cavalinho' },
    RCB3: { lat: -25.XXXX, lng: -49.XXXX, name: 'Reservatório B03' },
    CIE1: { lat: -25.XXXX, lng: -49.XXXX, name: 'Captação Ilha Engenho 01' },
    CIE2: { lat: -25.XXXX, lng: -49.XXXX, name: 'Captação Ilha Engenho 02' },
    RCAV2: { lat: -25.XXXX, lng: -49.XXXX, name: 'Reservatório Cavalinho 2' }
  };
  ```

- [ ] **Markers customizados:**
  - Cor por tipo (reservatório=azul, captação=verde, bomba=laranja)
  - Tamanho por capacidade
  - Pulsação para alertas
  - Tooltips com dados em tempo real

- [ ] **Conexões entre ativos:**
  - Linhas conectando tubulações
  - Setas indicando fluxo (animadas)
  - Espessura proporcional ao diâmetro
  - Cor indicando status (verde=ok, vermelho=alerta)

- [ ] **Painel lateral:**
  - Lista de todos os ativos
  - Filtros (tipo, status, região)
  - Busca rápida
  - Click para zoom no mapa

- [ ] **Info cards flutuantes:**
  - Click no marker abre modal detalhado
  - Volume atual, tendência, histórico 24h
  - Botão "Ver detalhes" → redireciona para SCADA específico

---

## 📊 3. PAINEL SCADA AVANÇADO

### 3.1 Dashboard Principal (`dashboard.php` / `scada.html`)
**Referência:** `.REFERENCIAS/templates/aqua-flow.html`

- [ ] **Status Bar no topo:**
  - Sistema operando normalmente ✅ / Em alerta ⚠️ / Offline 🔴
  - Volume total da rede
  - Consumo atual (L/h)
  - Alertas ativos (contador + dropdown)

- [ ] **Cards de reservatórios:**
  - Gauge visual de nível (circular ou barra)
  - Animação de água (wave effect)
  - Trend indicator (↗️ subindo, ↘️ descendo, → estável)
  - Last reading timestamp
  - Botão "Histórico" (modal com gráfico 24h)

- [ ] **Grid responsivo:**
  - Desktop: 3 colunas
  - Tablet: 2 colunas
  - Mobile: 1 coluna

### 3.2 Widgets Avançados
- [ ] **Tank Widget com SVG animado:**
  ```html
  <svg viewBox="0 0 200 300">
    <!-- Tank body -->
    <rect x="50" y="50" width="100" height="200" fill="#e0e0e0"/>
    <!-- Water fill (animated) -->
    <rect x="50" :y="250 - (level * 2)" width="100" :height="level * 2" fill="url(#waterGradient)">
      <animate attributeName="y" dur="2s" repeatCount="indefinite"/>
    </rect>
    <!-- Level text -->
    <text x="100" y="150" text-anchor="middle">{{level}}%</text>
  </svg>
  ```

- [ ] **Flow Meter Widget:**
  - Vazão em tempo real
  - Seta animada indicando direção
  - Total acumulado hoje

- [ ] **Valve Widget:**
  - Status aberta/fechada
  - Botão para comando manual (se aplicável)
  - Log de última mudança

### 3.3 Gráficos Interativos
**Referência:** `.REFERENCIAS/templates/aqua-flow.html` (canvas charts)

- [ ] **Biblioteca de charts:**
  - Opção 1: Chart.js (leve, simples)
  - Opção 2: ApexCharts (mais features)
  - Opção 3: Canvas customizado (mais controle)

- [ ] **Gráfico de consumo:**
  - Linha temporal (últimas 24h/7d/30d)
  - Área preenchida com gradient
  - Markers para eventos (abastecimento, alertas)
  - Zoom e pan interativos
  - Export PNG/CSV

- [ ] **Gráfico de balanço hídrico:**
  - Barras empilhadas (entrada vs saída)
  - Por reservatório ou total da rede
  - Comparação com média histórica

- [ ] **Distribuição de consumo:**
  - Doughnut chart por reservatório
  - Percentual de cada ativo no total

---

## 🎯 4. COMPONENTES REUTILIZÁVEIS

### 4.1 Criar biblioteca de componentes
**Referência:** `.REFERENCIAS/base44_aguada/components/`

- [ ] **`components/nav.html`** - Navegação lateral fixa
- [ ] **`components/header.html`** - Header com breadcrumb
- [ ] **`components/tank-widget.html`** - Widget de tanque
- [ ] **`components/chart-card.html`** - Card para gráficos
- [ ] **`components/alert-badge.html`** - Badge de alertas
- [ ] **`components/stat-card.html`** - Card de estatística
- [ ] **`components/filter-bar.html`** - Barra de filtros
- [ ] **`components/modal.html`** - Modal genérico

### 4.2 Sistema de includes
- [ ] Implementar SSI (Server Side Includes) ou
- [ ] JavaScript template system (fetch + innerHTML)
- [ ] Web Components (custom elements)

---

## 📱 5. RESPONSIVIDADE & PWA

### 5.1 Mobile First
- [ ] **Breakpoints:**
  - Mobile: < 640px
  - Tablet: 640px - 1024px
  - Desktop: > 1024px

- [ ] **Sidebar responsiva:**
  - Mobile: Hamburguer menu (slide-in)
  - Tablet/Desktop: Sempre visível

- [ ] **Cards adaptáveis:**
  - Mobile: Cards full-width, scroll vertical
  - Desktop: Grid layout

### 5.2 Progressive Web App
- [ ] **`manifest.json`**
  ```json
  {
    "name": "AGUADA - CMASM",
    "short_name": "AGUADA",
    "icons": [...],
    "start_url": "/",
    "display": "standalone",
    "theme_color": "#0066cc"
  }
  ```

- [ ] **Service Worker** (offline support)
- [ ] **Instalação no device** (Add to Home Screen)
- [ ] **Push notifications** para alertas críticos

---

## 🎨 6. PÁGINAS ESPECÍFICAS

### 6.1 Página de Consumo Detalhado
**Referência:** `.REFERENCIAS/base44_aguada/components/consumo.html`

- [ ] **Filtros avançados:**
  - Por período (hora, dia, semana, mês)
  - Por reservatório (multi-select)
  - Por tipo de consumo (normal, anormal, vazamento)

- [ ] **Relatório por quartos do dia:**
  - 00h-06h: Madrugada
  - 06h-12h: Manhã
  - 12h-18h: Tarde
  - 18h-00h: Noite

- [ ] **Tabela detalhada:**
  - Data/hora, reservatório, volume, tipo
  - Ordenação por coluna
  - Exportar CSV/Excel
  - Paginação (50/100/200 registros)

- [ ] **Cards de resumo:**
  - Total consumido no período
  - Média diária
  - Pico de consumo (horário)
  - Economia vs mês anterior

### 6.2 Página de Abastecimentos
**Referência:** `.REFERENCIAS/templates/abastecimento_detalhado.html`

- [ ] **Timeline de abastecimentos:**
  - Linha do tempo visual
  - Card para cada evento
  - Volume abastecido
  - Origem/destino
  - Responsável (se aplicável)

- [ ] **Formulário de registro:**
  - Data/hora
  - Reservatório destino
  - Volume (L)
  - Responsável
  - Observações
  - Upload de fotos

### 6.3 Página de Configurações
- [ ] **Aba Sensores:**
  - Lista de todos os nodes
  - Editar nome, calibração, limites
  - Testar comunicação
  - Ver log de erros

- [ ] **Aba Notificações:**
  - Configurar alertas (email, SMS, push)
  - Limites personalizados por reservatório
  - Horários de silêncio

- [ ] **Aba Usuários:**
  - Gerenciar acessos
  - Níveis de permissão (admin, operador, visualizador)

- [ ] **Aba Sistema:**
  - Backup banco de dados
  - Logs de sistema
  - Informações de hardware (gateway)

---

## 🚀 7. ANIMAÇÕES E INTERATIVIDADE

### 7.1 Micro-interações
- [ ] **Loading states:**
  - Skeleton screens
  - Spinners customizados
  - Progress bars

- [ ] **Hover effects:**
  - Cards levitam (+5px)
  - Botões mudam cor suavemente
  - Tooltips aparecem suavemente

- [ ] **Transições de página:**
  - Fade in/out
  - Slide between sections

### 7.2 Animações de dados
- [ ] **Números animados:**
  - Contadores que "rolam" até o valor
  - Biblioteca: CountUp.js

- [ ] **Gráficos animados:**
  - Barras/linhas crescem ao carregar
  - Smooth transitions ao mudar dados

- [ ] **Status indicators:**
  - Pulse animation para "online"
  - Shake para alertas
  - Wave effect para água

---

## 📦 8. ASSETS E RECURSOS

### 8.1 Ícones
- [ ] **Biblioteca de ícones:**
  - Opção 1: Lucide Icons (React/Vue)
  - Opção 2: Heroicons (Tailwind)
  - Opção 3: SVG customizados

- [ ] **Ícones necessários:**
  - 💧 Água/tanque
  - 📊 Gráficos
  - ⚙️ Configurações
  - 🗺️ Mapa
  - 🔔 Alertas
  - 📄 Relatórios
  - 🔌 Sensores
  - ⚡ Status

### 8.2 Imagens
- [ ] **Logo CMASM** (SVG vetorial)
- [ ] **Fotos dos reservatórios** (para tooltips)
- [ ] **Diagramas de rede** (topologia)

### 8.3 Fontes
- [ ] **Inter** (Google Fonts) - UI text
- [ ] **JetBrains Mono** - Código/dados técnicos

---

## 🔧 9. PERFORMANCE E OTIMIZAÇÃO

### 9.1 Otimização de carregamento
- [ ] **Lazy loading:**
  - Imagens (loading="lazy")
  - Gráficos (render on scroll)
  - Componentes (code splitting)

- [ ] **Minificação:**
  - CSS (cssnano)
  - JavaScript (terser)
  - HTML (html-minifier)

- [ ] **Caching:**
  - Service Worker cache
  - Browser cache headers
  - LocalStorage para preferências

### 9.2 Real-time updates
- [ ] **Polling inteligente:**
  - 5s para página ativa
  - 30s para página em background
  - Pausa quando tab inativa (Page Visibility API)

- [ ] **WebSocket** (futuro)
  - Server push para alertas instantâneos
  - Dados em tempo real sem polling

---

## 📚 10. DOCUMENTAÇÃO

### 10.1 Guias de uso
- [ ] **README.md** no frontend/
  - Como rodar localmente
  - Estrutura de arquivos
  - Como adicionar páginas

- [ ] **COMPONENTS.md**
  - Documentação de cada componente
  - Props, exemplos, screenshots

- [ ] **STYLE_GUIDE.md**
  - Paleta de cores
  - Tipografia
  - Espaçamentos
  - Padrões de código

### 10.2 Comentários no código
- [ ] JSDoc para funções JavaScript
- [ ] CSS comments para seções
- [ ] HTML comments para blocos complexos

---

## 🎯 PRIORIZAÇÃO

### ✅ FASE 1 - ESSENCIAL (Esta Sprint)
1. **Sidebar + Header unificados** (todas páginas)
2. **Mapa básico com markers** (SVG ou Leaflet)
3. **Dashboard SCADA melhorado** (cards + gauges)
4. **Página de consumo detalhado**

### 🟡 FASE 2 - IMPORTANTE (Próxima Sprint)
5. **Gráficos interativos** (Chart.js)
6. **Componentes reutilizáveis**
7. **Página de configurações**
8. **Responsividade mobile**

### 🔵 FASE 3 - DESEJÁVEL (Backlog)
9. **PWA + Service Worker**
10. **Animações avançadas**
11. **WebSocket real-time**
12. **Dark mode completo**

---

## 📋 CHECKLIST DE ARQUIVOS A CRIAR

```
frontend/
├── mapa.html              # ✅ Página de mapa interativo
├── consumo.html           # ✅ Consumo detalhado
├── abastecimentos.html    # ✅ Timeline de abastecimentos
├── configuracoes.html     # ✅ Painel de configurações
├── components/
│   ├── nav.html           # ✅ Sidebar navigation
│   ├── header.html        # ✅ Top header
│   ├── tank-widget.html   # ✅ Widget de tanque
│   ├── chart-card.html    # ✅ Card para gráficos
│   └── modal.html         # ✅ Modal genérico
├── assets/
│   ├── css/
│   │   ├── variables.css  # ✅ CSS variables
│   │   ├── components.css # ✅ Componentes
│   │   └── animations.css # ✅ Animações
│   ├── js/
│   │   ├── charts.js      # ✅ Gráficos
│   │   ├── map.js         # ✅ Mapa
│   │   └── utils.js       # ✅ Utilidades
│   ├── icons/             # ✅ SVG icons
│   └── images/            # ✅ Fotos dos ativos
├── manifest.json          # ✅ PWA manifest
└── service-worker.js      # ✅ Service worker
```

---

## 🎨 INSPIRAÇÃO VISUAL

**Referências de design:**
- `.REFERENCIAS/templates/aqua-flow.html` → Background animado, glassmorphism
- `.REFERENCIAS/templates/painel.html` → Cards com hover effects
- `.REFERENCIAS/MapaPage.tsx` → Mapa com SVG e cores por status
- `.REFERENCIAS/tailadmin-*/` → Layout moderno, sidebar dark
- `.REFERENCIAS/base44_aguada/` → Navegação clean, componentes React

**Color schemes:**
- Azul água (#0066cc, #00a8ff) - Primary
- Verde (#00c853) - Success/Online
- Amarelo (#ffd600) - Warning
- Vermelho (#ff1744) - Critical/Offline
- Dark mode (#1a1a2e, #16213e) - Backgrounds

---

**🚀 OBJETIVO FINAL:**  
Dashboard profissional e moderno para monitoramento de rede hidráulica com:
- Visualização intuitiva de dados em tempo real
- Mapa interativo com localização dos ativos
- Gráficos para análise de tendências
- Interface responsiva (mobile/tablet/desktop)
- Animações suaves e micro-interações
- Performance otimizada com lazy loading

**👨‍💻 TECNOLOGIAS:**
- HTML5 + CSS3 (Tailwind ou custom)
- JavaScript (ES6+, Alpine.js opcional)
- Chart.js ou ApexCharts
- Leaflet.js para mapas
- LocalStorage para persistência
- Service Worker para PWA

**📅 ESTIMATIVA:**
- Fase 1: 2-3 dias
- Fase 2: 3-4 dias
- Fase 3: 2-3 dias
- **Total: 7-10 dias de desenvolvimento**
