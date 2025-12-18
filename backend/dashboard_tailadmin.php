<?php
require_once __DIR__ . '/config.php';
$mysqli = db_connect();

// Load sensors dynamically from database
$sensors = [];
$sensor_query = $mysqli->query("
    SELECT s.node_id, s.alias, e.nome as descricao, e.capacidade_l 
    FROM sensores s 
    LEFT JOIN elementos e ON s.elemento_id = e.id 
    ORDER BY s.node_id
");

if ($sensor_query) {
    while ($s = $sensor_query->fetch_assoc()) {
        $icons = ['🏊', '💧', '🌊', '💦', '⚙️', '🚰']; // Icon rotation
        $icon_index = ($s['node_id'] - 1) % count($icons);
        
        $sensors[$s['node_id']] = [
            'alias' => $s['alias'],
            'desc' => $s['descricao'],
            'icon' => $icons[$icon_index],
            'capacity' => (int)$s['capacidade_l']
        ];
    }
}

$result = $mysqli->query('SELECT * FROM leituras_v2 ORDER BY created_at DESC LIMIT 50');
$rows = $result ? $result->fetch_all(MYSQLI_ASSOC) : [];

// Get latest reading per node
$summary = [];
foreach ($rows as $r) {
    $nid = $r['node_id'];
    if (!isset($summary[$nid])) {
        $summary[$nid] = [];
    }
    $summary[$nid][] = $r;
}

function signal_quality($rssi) {
    if ($rssi > -60) return ['quality' => 'Excelente', 'color' => 'success'];
    if ($rssi > -70) return ['quality' => 'Bom', 'color' => 'warning'];
    if ($rssi > -80) return ['quality' => 'Fraco', 'color' => 'error'];
    return ['quality' => 'Crítico', 'color' => 'error'];
}

function battery_status($mv) {
    if ($mv > 4100) return ['status' => 'OK', 'color' => 'success'];
    if ($mv > 3800) return ['status' => 'Baixa', 'color' => 'warning'];
    return ['status' => 'Crítica', 'color' => 'error'];
}
?><!doctype html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <meta http-equiv="refresh" content="30">
    <title>AGUADA - Dashboard de Telemetria</title>
    <script src="https://cdn.tailwindcss.com"></script>
    <script defer src="https://cdn.jsdelivr.net/npm/alpinejs@3.x.x/dist/cdn.min.js"></script>
    <script>
        tailwind.config = {
            darkMode: 'class',
            theme: {
                extend: {
                    colors: {
                        primary: '#3C50E0',
                        'gray': {
                            '50': '#f9fafb',
                            '100': '#f3f4f6',
                            '200': '#e5e7eb',
                            '300': '#d1d5db',
                            '400': '#9ca3af',
                            '500': '#6b7280',
                            '600': '#4b5563',
                            '700': '#374151',
                            '800': '#1f2937',
                            '900': '#111827',
                        },
                        success: {
                            '50': '#ecfdf5',
                            '500': '#10b981',
                            '600': '#059669',
                        },
                        warning: {
                            '50': '#fffbeb',
                            '500': '#f59e0b',
                            '600': '#d97706',
                        },
                        error: {
                            '50': '#fef2f2',
                            '500': '#ef4444',
                            '600': '#dc2626',
                        }
                    }
                }
            }
        }
    </script>
    <style>
        [x-cloak] { display: none !important; }
    </style>
</head>
<body 
    x-data="{ 
        darkMode: false, 
        sidebarToggle: false 
    }"
    x-init="
        darkMode = JSON.parse(localStorage.getItem('darkMode') || 'false');
        $watch('darkMode', value => localStorage.setItem('darkMode', JSON.stringify(value)))
    "
    :class="{'dark bg-gray-900': darkMode === true}"
    class="font-sans antialiased"
>
    <!-- Page Wrapper -->
    <div class="flex h-screen overflow-hidden">
        
        <!-- Sidebar -->
        <aside
            :class="sidebarToggle ? 'translate-x-0' : '-translate-x-full lg:translate-x-0'"
            class="fixed left-0 top-0 z-50 flex h-screen w-72 flex-col overflow-y-hidden border-r border-gray-200 bg-white transition-transform duration-300 dark:border-gray-800 dark:bg-black lg:static"
        >
            <!-- Logo -->
            <div class="flex items-center justify-between gap-2 px-6 py-6 lg:py-7">
                <a href="index.php" class="flex items-center gap-2">
                    <span class="text-2xl font-bold text-primary">💧 AGUADA</span>
                </a>
            </div>

            <!-- Nav -->
            <nav class="px-4 py-4">
                <h3 class="mb-4 ml-4 text-xs font-semibold uppercase text-gray-400">Menu</h3>
                
                <ul class="space-y-1.5">
                    <li>
                        <a href="dashboard_tailadmin.php" class="group relative flex items-center gap-2.5 rounded-lg px-4 py-2 font-medium text-white bg-primary">
                            <svg class="fill-current" width="18" height="18" viewBox="0 0 18 18"><path d="M6.10322 0.956299H2.53135C1.5751 0.956299 0.787598 1.7438 0.787598 2.70005V6.27192C0.787598 7.22817 1.5751 8.01567 2.53135 8.01567H6.10322C7.05947 8.01567 7.84697 7.22817 7.84697 6.27192V2.72817C7.8751 1.7438 7.0876 0.956299 6.10322 0.956299ZM6.60947 6.30005C6.60947 6.5813 6.38447 6.8063 6.10322 6.8063H2.53135C2.2501 6.8063 2.0251 6.5813 2.0251 6.30005V2.72817C2.0251 2.44692 2.2501 2.22192 2.53135 2.22192H6.10322C6.38447 2.22192 6.60947 2.44692 6.60947 2.72817V6.30005Z"/><path d="M15.4689 0.956299H11.8971C10.9408 0.956299 10.1533 1.7438 10.1533 2.70005V6.27192C10.1533 7.22817 10.9408 8.01567 11.8971 8.01567H15.4689C16.4252 8.01567 17.2127 7.22817 17.2127 6.27192V2.72817C17.2127 1.7438 16.4252 0.956299 15.4689 0.956299ZM15.9752 6.30005C15.9752 6.5813 15.7502 6.8063 15.4689 6.8063H11.8971C11.6158 6.8063 11.3908 6.5813 11.3908 6.30005V2.72817C11.3908 2.44692 11.6158 2.22192 11.8971 2.22192H15.4689C15.7502 2.22192 15.9752 2.44692 15.9752 2.72817V6.30005Z"/><path d="M6.10322 9.92822H2.53135C1.5751 9.92822 0.787598 10.7157 0.787598 11.672V15.2438C0.787598 16.2001 1.5751 16.9876 2.53135 16.9876H6.10322C7.05947 16.9876 7.84697 16.2001 7.84697 15.2438V11.7001C7.8751 10.7157 7.0876 9.92822 6.10322 9.92822ZM6.60947 15.272C6.60947 15.5532 6.38447 15.7782 6.10322 15.7782H2.53135C2.2501 15.7782 2.0251 15.5532 2.0251 15.272V11.7001C2.0251 11.4188 2.2501 11.1938 2.53135 11.1938H6.10322C6.38447 11.1938 6.60947 11.4188 6.60947 11.7001V15.272Z"/><path d="M15.4689 9.92822H11.8971C10.9408 9.92822 10.1533 10.7157 10.1533 11.672V15.2438C10.1533 16.2001 10.9408 16.9876 11.8971 16.9876H15.4689C16.4252 16.9876 17.2127 16.2001 17.2127 15.2438V11.7001C17.2127 10.7157 16.4252 9.92822 15.4689 9.92822ZM15.9752 15.272C15.9752 15.5532 15.7502 15.7782 15.4689 15.7782H11.8971C11.6158 15.7782 11.3908 15.5532 11.3908 15.272V11.7001C11.3908 11.4188 11.6158 11.1938 11.8971 11.1938H15.4689C15.7502 11.1938 15.9752 11.4188 15.9752 11.7001V15.272Z"/></svg>
                            Dashboard
                        </a>
                    </li>
                    <li>
                        <a href="relatorio_servico.html" class="group relative flex items-center gap-2.5 rounded-lg px-4 py-2 font-medium text-gray-600 hover:bg-gray-100 dark:text-gray-300 dark:hover:bg-gray-800">
                            <svg class="fill-current" width="18" height="18" viewBox="0 0 18 18"><path d="M15.7499 2.9812H14.2874V2.36245C14.2874 2.02495 14.0062 1.71558 13.6405 1.71558C13.2749 1.71558 12.9937 1.99683 12.9937 2.36245V2.9812H4.97803V2.36245C4.97803 2.02495 4.69678 1.71558 4.33115 1.71558C3.96553 1.71558 3.68428 1.99683 3.68428 2.36245V2.9812H2.2499C1.29365 2.9812 0.478027 3.7687 0.478027 4.75308V14.5406C0.478027 15.4968 1.26553 16.3125 2.2499 16.3125H15.7499C16.7062 16.3125 17.5218 15.525 17.5218 14.5406V4.72495C17.5218 3.7687 16.7062 2.9812 15.7499 2.9812ZM1.77178 8.21245H4.1624V10.9968H1.77178V8.21245ZM5.42803 8.21245H8.38115V10.9968H5.42803V8.21245ZM8.38115 12.2625V15.0187H5.42803V12.2625H8.38115ZM9.64678 12.2625H12.5999V15.0187H9.64678V12.2625ZM9.64678 10.9968V8.21245H12.5999V10.9968H9.64678ZM13.8374 8.21245H16.228V10.9968H13.8374V8.21245ZM2.2499 4.24683H3.7124V4.83745C3.7124 5.17495 3.99365 5.48433 4.35928 5.48433C4.7249 5.48433 5.00615 5.20308 5.00615 4.83745V4.24683H13.0499V4.83745C13.0499 5.17495 13.3312 5.48433 13.6968 5.48433C14.0624 5.48433 14.3437 5.20308 14.3437 4.83745V4.24683H15.7499C16.0312 4.24683 16.2562 4.47183 16.2562 4.75308V6.94683H1.77178V4.75308C1.77178 4.47183 1.96865 4.24683 2.2499 4.24683ZM1.77178 14.5125V12.2343H4.1624V14.9906H2.2499C1.96865 15.0187 1.77178 14.7937 1.77178 14.5125ZM15.7499 15.0187H13.8374V12.2625H16.228V14.5406C16.2562 14.7937 16.0312 15.0187 15.7499 15.0187Z"/></svg>
                            Balanço Hídrico
                        </a>
                    </li>
                    <li>
                        <a href="scada.html" class="group relative flex items-center gap-2.5 rounded-lg px-4 py-2 font-medium text-gray-600 hover:bg-gray-100 dark:text-gray-300 dark:hover:bg-gray-800">
                            <svg class="fill-current" width="18" height="18" viewBox="0 0 18 18"><path d="M9.0002 7.79065C8.27826 7.79065 7.67184 8.39706 7.67184 9.119C7.67184 9.84094 8.27826 10.4474 9.0002 10.4474C9.72213 10.4474 10.3285 9.84094 10.3285 9.119C10.3285 8.39706 9.72213 7.79065 9.0002 7.79065ZM9.0002 11.8283C7.50826 11.8283 6.29102 10.611 6.29102 9.119C6.29102 7.62706 7.50826 6.40983 9.0002 6.40983C10.4921 6.40983 11.7093 7.62706 11.7093 9.119C11.7093 10.611 10.4921 11.8283 9.0002 11.8283Z"/><path d="M11.9496 13.4542C11.8433 13.4542 11.7371 13.4261 11.6589 13.3417L9.19022 11.2136C9.0558 11.1011 8.97772 10.9386 8.97772 10.7761V5.67356C8.97772 5.37606 9.21834 5.13544 9.51584 5.13544C9.81334 5.13544 10.054 5.37606 10.054 5.67356V10.4755L12.268 12.378C12.4867 12.5686 12.5086 12.903 12.318 13.1217C12.2117 13.2842 12.0492 13.4542 11.9496 13.4542Z"/><path d="M15.3749 17.9999H2.6249C1.29365 17.9999 0.224854 16.9311 0.224854 15.5999V3.59991C0.224854 2.26866 1.29365 1.19991 2.6249 1.19991H4.89053V0.806159C4.89053 0.508659 5.13115 0.268036 5.42865 0.268036C5.72615 0.268036 5.96678 0.508659 5.96678 0.806159V1.19991H12.0374V0.806159C12.0374 0.508659 12.278 0.268036 12.5755 0.268036C12.873 0.268036 13.1136 0.508659 13.1136 0.806159V1.19991H15.3749C16.7061 1.19991 17.7749 2.26866 17.7749 3.59991V15.5999C17.7749 16.9311 16.7061 17.9999 15.3749 17.9999ZM2.6249 2.58116C1.96678 2.58116 1.4249 3.12303 1.4249 3.78116V15.5999C1.4249 16.258 1.96678 16.7999 2.6249 16.7999H15.3749C16.033 16.7999 16.5749 16.258 16.5749 15.5999V3.78116C16.5749 3.12303 16.033 2.58116 15.3749 2.58116H2.6249Z"/></svg>
                            SCADA
                        </a>
                    </li>
                </ul>
            </nav>
        </aside>

        <!-- Content Area -->
        <div class="relative flex flex-1 flex-col overflow-y-auto overflow-x-hidden">
            
            <!-- Header -->
            <header class="sticky top-0 z-40 flex w-full border-b border-gray-200 bg-white dark:border-gray-800 dark:bg-gray-900">
                <div class="flex flex-grow items-center justify-between px-4 py-4 shadow-sm md:px-6 2xl:px-11">
                    <div class="flex items-center gap-2 sm:gap-4 lg:hidden">
                        <button
                            @click="sidebarToggle = !sidebarToggle"
                            class="z-50 block rounded-sm border border-gray-200 bg-white p-1.5 shadow-sm lg:hidden dark:border-gray-800 dark:bg-gray-800"
                        >
                            <svg class="fill-current" width="20" height="20" viewBox="0 0 20 20"><path fill-rule="evenodd" clip-rule="evenodd" d="M3 5C3 4.58579 3.33579 4.25 3.75 4.25H16.25C16.6642 4.25 17 4.58579 17 5C17 5.41421 16.6642 5.75 16.25 5.75H3.75C3.33579 5.75 3 5.41421 3 5ZM3 15C3 14.5858 3.33579 14.25 3.75 14.25H16.25C16.6642 14.25 17 14.5858 17 15C17 15.4142 16.6642 15.75 16.25 15.75H3.75C3.33579 15.75 3 15.4142 3 15ZM3.75 9.25C3.33579 9.25 3 9.58579 3 10C3 10.4142 3.33579 10.75 3.75 10.75H10C10.4142 10.75 10.75 10.4142 10.75 10C10.75 9.58579 10.4142 9.25 10 9.25H3.75Z"/></svg>
                        </button>
                    </div>

                    <div class="flex items-center gap-3">
                        <span class="text-sm text-gray-600 dark:text-gray-400">Auto-atualiza a cada 30s | Última: <?php echo date('H:i:s'); ?></span>
                    </div>

                    <div class="flex items-center gap-3 2xsm:gap-7">
                        <button
                            @click="darkMode = !darkMode"
                            class="flex items-center justify-center rounded-full border border-gray-200 bg-gray-100 px-3 py-1.5 text-sm font-medium hover:bg-gray-200 dark:border-gray-700 dark:bg-gray-800 dark:hover:bg-gray-700"
                        >
                            <span x-show="!darkMode" class="flex items-center gap-1">
                                <svg class="fill-current" width="16" height="16" viewBox="0 0 16 16"><path d="M8 0C8.27614 0 8.5 0.223858 8.5 0.5V2.5C8.5 2.77614 8.27614 3 8 3C7.72386 3 7.5 2.77614 7.5 2.5V0.5C7.5 0.223858 7.72386 0 8 0Z"/><path d="M11.5 8C11.5 9.933 9.933 11.5 8 11.5C6.067 11.5 4.5 9.933 4.5 8C4.5 6.067 6.067 4.5 8 4.5C9.933 4.5 11.5 6.067 11.5 8Z"/></svg>
                                Escuro
                            </span>
                            <span x-show="darkMode" class="flex items-center gap-1" x-cloak>
                                <svg class="fill-current" width="16" height="16" viewBox="0 0 16 16"><path d="M8 0C8.27614 0 8.5 0.223858 8.5 0.5V2.5C8.5 2.77614 8.27614 3 8 3C7.72386 3 7.5 2.77614 7.5 2.5V0.5C7.5 0.223858 7.72386 0 8 0Z"/></svg>
                                Claro
                            </span>
                        </button>
                    </div>
                </div>
            </header>

            <!-- Main Content -->
            <main>
                <div class="mx-auto max-w-screen-2xl p-4 md:p-6 2xl:p-10">
                    
                    <div class="mb-6">
                        <h2 class="text-2xl font-semibold text-gray-800 dark:text-white">Dashboard de Telemetria</h2>
                        <p class="text-sm text-gray-500 dark:text-gray-400">Monitoramento em tempo real dos reservatórios de água</p>
                    </div>

                    <!-- Metrics Grid -->
                    <div class="grid grid-cols-1 gap-4 md:grid-cols-2 md:gap-6 xl:grid-cols-3 2xl:gap-7.5 mb-6">
                        <?php foreach ($summary as $nid => $readings):
                            $latest_r = $readings[0];
                            $sensor = $sensors[$nid] ?? ['alias' => "N$nid", 'desc' => 'Sensor', 'icon' => '📊', 'capacity' => 0];
                            $pct = (int)$latest_r['percentual'];
                            $volume = (int)$latest_r['volume_l'];
                            $volume_m3 = round($volume / 1000, 1); // Convert liters to m³
                            $capacity = $sensor['capacity'];
                            $capacity_m3 = round($capacity / 1000, 1);
                            $level = (int)$latest_r['level_cm'];
                            $sig = signal_quality((int)$latest_r['rssi']);
                            $bat = battery_status((int)$latest_r['vin_mv']);
                            
                            $pct_color = $pct > 75 ? 'success' : ($pct > 50 ? 'warning' : 'error');
                        ?>
                        <!-- Card -->
                        <div class="rounded-2xl border border-gray-200 bg-white p-6 dark:border-gray-800 dark:bg-white/[0.03]">
                            <div class="flex items-start justify-between">
                                <div class="flex h-12 w-12 items-center justify-center rounded-xl bg-gray-100 dark:bg-gray-800 text-2xl">
                                    <?php echo $sensor['icon']; ?>
                                </div>
                                
                                <span class="flex items-center gap-1 rounded-full bg-<?php echo $pct_color; ?>-50 px-2 py-1 text-xs font-medium text-<?php echo $pct_color; ?>-600 dark:bg-<?php echo $pct_color; ?>-500/15 dark:text-<?php echo $pct_color; ?>-500">
                                    <?php echo $pct; ?>%
                                </span>
                            </div>

                            <div class="mt-4">
                                <h4 class="text-sm font-semibold text-primary"><?php echo htmlspecialchars($sensor['alias']); ?></h4>
                                <p class="text-xs text-gray-500 dark:text-gray-400"><?php echo htmlspecialchars($sensor['desc']); ?></p>
                                
                                <div class="mt-3 flex items-baseline gap-2">
                                    <span class="text-2xl font-bold text-gray-800 dark:text-white"><?php echo $volume_m3; ?></span>
                                    <span class="text-sm text-gray-500 dark:text-gray-400">m³</span>
                                    <span class="text-xs text-gray-400 dark:text-gray-500">/ <?php echo $capacity_m3; ?> m³</span>
                                </div>
                                
                                <p class="mt-1 text-xs text-gray-500 dark:text-gray-400">Nível: <?php echo $level; ?> cm</p>

                                <!-- Progress Bar -->
                                <div class="mt-3 h-2 w-full rounded-full bg-gray-100 dark:bg-gray-800">
                                    <div class="h-2 rounded-full bg-<?php echo $pct_color; ?>-500" style="width: <?php echo $pct; ?>%"></div>
                                </div>

                                <!-- Status badges -->
                                <div class="mt-3 flex items-center gap-2 text-xs">
                                    <span class="inline-flex items-center gap-1 rounded-full bg-<?php echo $sig['color']; ?>-50 px-2 py-0.5 text-<?php echo $sig['color']; ?>-600 dark:bg-<?php echo $sig['color']; ?>-500/15 dark:text-<?php echo $sig['color']; ?>-500">
                                        📡 <?php echo $sig['quality']; ?>
                                    </span>
                                    <span class="inline-flex items-center gap-1 rounded-full bg-<?php echo $bat['color']; ?>-50 px-2 py-0.5 text-<?php echo $bat['color']; ?>-600 dark:bg-<?php echo $bat['color']; ?>-500/15 dark:text-<?php echo $bat['color']; ?>-500">
                                        🔋 <?php echo $bat['status']; ?>
                                    </span>
                                </div>
                            </div>
                        </div>
                        <?php endforeach; ?>
                    </div>

                    <!-- Table -->
                    <div class="rounded-2xl border border-gray-200 bg-white dark:border-gray-800 dark:bg-white/[0.03]">
                        <div class="px-6 py-6 border-b border-gray-200 dark:border-gray-800">
                            <h3 class="text-lg font-semibold text-gray-800 dark:text-white">Histórico de Leituras</h3>
                            <p class="text-sm text-gray-500 dark:text-gray-400">Últimas 50 leituras dos sensores</p>
                        </div>

                        <div class="overflow-x-auto">
                            <table class="w-full table-auto">
                                <thead>
                                    <tr class="bg-gray-50 text-left dark:bg-gray-800">
                                        <th class="px-4 py-4 text-xs font-medium uppercase text-gray-600 dark:text-gray-400">Data/Hora</th>
                                        <th class="px-4 py-4 text-xs font-medium uppercase text-gray-600 dark:text-gray-400">Reservatório</th>
                                        <th class="px-4 py-4 text-xs font-medium uppercase text-gray-600 dark:text-gray-400">Node</th>
                                        <th class="px-4 py-4 text-xs font-medium uppercase text-gray-600 dark:text-gray-400">Nível</th>
                                        <th class="px-4 py-4 text-xs font-medium uppercase text-gray-600 dark:text-gray-400">Volume</th>
                                        <th class="px-4 py-4 text-xs font-medium uppercase text-gray-600 dark:text-gray-400">Capacidade</th>
                                        <th class="px-4 py-4 text-xs font-medium uppercase text-gray-600 dark:text-gray-400">Sinal</th>
                                        <th class="px-4 py-4 text-xs font-medium uppercase text-gray-600 dark:text-gray-400">Bateria</th>
                                    </tr>
                                </thead>
                                <tbody>
                                    <?php foreach ($rows as $r):
                                        $sensor = $sensors[(int)$r['node_id']] ?? ['alias' => 'N' . (int)$r['node_id'], 'desc' => 'N/A', 'icon' => '📊'];
                                        $sig = signal_quality((int)$r['rssi']);
                                        $bat = battery_status((int)$r['vin_mv']);
                                        $pct = (int)$r['percentual'];
                                        $pct_color = $pct > 75 ? 'success' : ($pct > 50 ? 'warning' : 'error');
                                    ?>
                                    <tr class="border-b border-gray-200 dark:border-gray-800 hover:bg-gray-50 dark:hover:bg-gray-800/50">
                                        <td class="px-4 py-4">
                                            <p class="text-sm text-gray-600 dark:text-gray-400"><?php echo htmlspecialchars($r['created_at']); ?></p>
                                        </td>
                                        <td class="px-4 py-4">
                                            <div class="flex items-center gap-2">
                                                <span class="text-lg"><?php echo $sensor['icon']; ?></span>
                                                <div>
                                                    <p class="text-sm font-medium text-gray-800 dark:text-white"><?php echo htmlspecialchars($sensor['alias']); ?></p>
                                                    <p class="text-xs text-gray-500 dark:text-gray-400"><?php echo htmlspecialchars($sensor['desc']); ?></p>
                                                </div>
                                            </div>
                                        </td>
                                        <td class="px-4 py-4">
                                            <span class="font-medium text-primary"><?php echo (int)$r['node_id']; ?></span>
                                        </td>
                                        <td class="px-4 py-4">
                                            <span class="text-sm text-gray-600 dark:text-gray-400"><?php echo (int)$r['level_cm']; ?> cm</span>
                                        </td>
                                        <td class="px-4 py-4">
                                            <span class="text-sm font-semibold text-gray-800 dark:text-white"><?php echo number_format((int)$r['volume_l'], 0, ',', '.'); ?> L</span>
                                        </td>
                                        <td class="px-4 py-4">
                                            <span class="inline-flex items-center gap-1 rounded-full bg-<?php echo $pct_color; ?>-50 px-2.5 py-0.5 text-sm font-medium text-<?php echo $pct_color; ?>-600 dark:bg-<?php echo $pct_color; ?>-500/15 dark:text-<?php echo $pct_color; ?>-500">
                                                <?php echo $pct; ?>%
                                            </span>
                                        </td>
                                        <td class="px-4 py-4">
                                            <span class="text-xs text-gray-600 dark:text-gray-400"><?php echo $sig['quality']; ?></span>
                                        </td>
                                        <td class="px-4 py-4">
                                            <span class="text-xs text-<?php echo $bat['color']; ?>-600 dark:text-<?php echo $bat['color']; ?>-500"><?php echo $bat['status']; ?> (<?php echo (int)$r['vin_mv']; ?>mV)</span>
                                        </td>
                                    </tr>
                                    <?php endforeach; ?>
                                </tbody>
                            </table>
                        </div>
                    </div>

                </div>
            </main>
        </div>
    </div>
</body>
</html>
