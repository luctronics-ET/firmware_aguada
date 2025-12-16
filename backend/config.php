<?php
// Configuração do banco
$DB_HOST = 'localhost';
$DB_USER = 'aguada_user';
$DB_PASS = '';
$DB_NAME = 'sensores_db';

// Conexão helper
function db_connect() {
    global $DB_HOST, $DB_USER, $DB_PASS, $DB_NAME;
    
    $mysqli = @new mysqli($DB_HOST, $DB_USER, $DB_PASS, $DB_NAME);
    
    if ($mysqli->connect_errno) {
        http_response_code(500);
        die('DB connection failed: ' . $mysqli->connect_error);
    }
    $mysqli->set_charset('utf8mb4');
    return $mysqli;
}
