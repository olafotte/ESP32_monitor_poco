#ifndef INDEX_H
#define INDEX_H

#include <Arduino.h>

const char index_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <title>Monitor de Poço & Caixa d'Água</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="utf-8">
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <style>
    :root {
      --bg: #0f172a;
      --card-bg: #1e293b;
      --text-main: #f8fafc;
      --text-sub: #94a3b8;
      --accent: #38bdf8;
      --accent-green: #22c55e;
      --accent-yellow: #eab308;
      --accent-red: #ef4444;
      --border: #334155;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; }
    body { background-color: var(--bg); color: var(--text-main); padding: 20px; min-height: 100vh; }
    .container { max-width: 800px; margin: 0 auto; display: flex; flex-direction: column; gap: 20px; }
    
    header { text-align: center; margin-bottom: 10px; }
    header h1 { font-size: 1.8rem; color: var(--text-main); display: flex; align-items: center; justify-content: center; gap: 10px; }
    header p { color: var(--text-sub); font-size: 0.9rem; margin-top: 5px; }

    .grid-cards { display: grid; grid-template-columns: repeat(auto-fit, minmax(220px, 1fr)); gap: 15px; }
    .card { background: var(--card-bg); border: 1px solid var(--border); border-radius: 12px; padding: 20px; text-align: center; box-shadow: 0 4px 12px rgba(0,0,0,0.2); }
    .card .label { font-size: 0.85rem; color: var(--text-sub); text-transform: uppercase; letter-spacing: 0.5px; }
    .card .value { font-size: 2.2rem; font-weight: bold; margin: 10px 0; color: var(--accent); }
    .card .subvalue { font-size: 0.85rem; color: var(--text-sub); }

    .tank-container { background: var(--card-bg); border: 1px solid var(--border); border-radius: 12px; padding: 20px; }
    .tank-title { font-size: 1rem; margin-bottom: 15px; text-align: center; color: var(--text-sub); }
    .tank-bar-bg { width: 100%; height: 28px; background: #0f172a; border-radius: 14px; overflow: hidden; border: 1px solid var(--border); position: relative; }
    .tank-bar-fill { height: 100%; width: 0%; background: linear-gradient(90deg, #0284c7, #38bdf8); transition: width 0.8s ease-in-out; border-radius: 14px; }
    .tank-bar-text { position: absolute; top: 0; left: 0; width: 100%; height: 100%; display: flex; align-items: center; justify-content: center; font-weight: bold; font-size: 0.9rem; text-shadow: 0 1px 2px rgba(0,0,0,0.8); }

    .chart-box { background: var(--card-bg); border: 1px solid var(--border); border-radius: 12px; padding: 20px; }
    .chart-box h3 { font-size: 1rem; color: var(--text-sub); margin-bottom: 15px; text-align: center; }

    .status-badge { display: inline-block; padding: 4px 12px; border-radius: 20px; font-weight: bold; font-size: 0.85rem; }
    .status-ok { background: rgba(34, 197, 94, 0.2); color: var(--accent-green); border: 1px solid var(--accent-green); }
    .status-warn { background: rgba(234, 179, 8, 0.2); color: var(--accent-yellow); border: 1px solid var(--accent-yellow); }
    .status-danger { background: rgba(239, 68, 68, 0.2); color: var(--accent-red); border: 1px solid var(--accent-red); }

    footer { text-align: center; color: var(--text-sub); font-size: 0.8rem; margin-top: 10px; }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <h1>💧 Monitor do Poço</h1>
      <p>Sistema de Telemetria Ultrassônica ESP32</p>
    </header>

    <div class="grid-cards">
      <div class="card">
        <div class="label">Nível da Água</div>
        <div class="value" id="valPct">-- %</div>
        <div class="subvalue" id="statusBadge"><span class="status-badge status-ok">Aguardando...</span></div>
      </div>
      <div class="card">
        <div class="label">Distância do Sensor</div>
        <div class="value" id="valDist">-- cm</div>
        <div class="subvalue">Distância útil até a superfície</div>
      </div>
      <div class="card">
        <div class="label">Configuração de Som</div>
        <label style="display:flex; align-items:center; justify-content:center; gap:10px; margin-top:15px; cursor:pointer;">
          <input type="checkbox" id="chkBeep" onchange="alterarBeep(this.checked)" style="width:20px; height:20px; cursor:pointer; accent-color:#38bdf8;">
          <span style="font-size:0.95rem; font-weight:bold;">🔊 Beep nas Leituras</span>
        </label>
      </div>
    </div>

    <div class="tank-container">
      <div class="tank-title">Volume Atual da Caixa / Poço</div>
      <div class="tank-bar-bg">
        <div class="tank-bar-fill" id="tankFill"></div>
        <div class="tank-bar-text" id="tankText">0%</div>
      </div>
    </div>

    <div class="chart-box">
      <h3>Histórico Recente de Distância (cm)</h3>
      <canvas id="graficoNivel"></canvas>
    </div>

    <div class="chart-box" style="margin-top: 15px;">
      <h3 style="display:flex; align-items:center; justify-content:center; gap:8px;">⚙️ Configurações do Sistema</h3>
      <div style="display:grid; grid-template-columns: repeat(auto-fit, minmax(170px, 1fr)); gap:12px; margin-top:15px;">
        <div>
          <label style="font-size:0.8rem; color:var(--text-sub);">Amostras Ultrassônicas (TOTAL_LEITURAS)</label>
          <input type="number" id="cfgTotLeituras" min="1" max="20" style="width:100%; padding:8px; border-radius:6px; border:1px solid var(--border); background:#0f172a; color:#fff; margin-top:4px;">
        </div>
        <div>
          <label style="font-size:0.8rem; color:var(--text-sub);">Tamanho do Histórico (MAX_LEITURAS)</label>
          <input type="number" id="cfgMaxLeituras" min="3" max="50" style="width:100%; padding:8px; border-radius:6px; border:1px solid var(--border); background:#0f172a; color:#fff; margin-top:4px;">
        </div>
        <div>
          <label style="font-size:0.8rem; color:var(--text-sub);">Intervalo Leitura Local (s)</label>
          <input type="number" id="cfgIntLeitura" min="3" style="width:100%; padding:8px; border-radius:6px; border:1px solid var(--border); background:#0f172a; color:#fff; margin-top:4px;">
        </div>
        <div>
          <label style="font-size:0.8rem; color:var(--text-sub);">Intervalo Envio Turso (min)</label>
          <input type="number" id="cfgIntTurso" min="1" style="width:100%; padding:8px; border-radius:6px; border:1px solid var(--border); background:#0f172a; color:#fff; margin-top:4px;">
        </div>
      </div>
      <div style="text-align:center; margin-top:15px;">
        <button onclick="salvarParametrosWeb()" style="background:var(--accent); color:#000; font-weight:bold; border:none; padding:10px 20px; border-radius:8px; cursor:pointer;">💾 Salvar Parâmetros</button>
        <span id="msgSalvarParam" style="display:block; font-size:0.85rem; color:var(--accent-green); margin-top:8px;"></span>
      </div>
    </div>

    <footer>
      ESP32 Local Server &bull; Atualizado a cada 5s &bull; mDNS: monitorpoco.local
    </footer>
  </div>

  <script>
    let meuGrafico;
    let usuarioAlterandoBeep = false;
    let usuarioEditandoCampos = false;

    async function alterarBeep(ativo) {
      try {
        usuarioAlterandoBeep = true;
        await fetch('/config_beep?ativo=' + (ativo ? '1' : '0'), { method: 'POST' });
        setTimeout(() => { usuarioAlterandoBeep = false; }, 2000);
      } catch (err) {
        console.error("Erro ao alterar configuracao do Beep:", err);
        usuarioAlterandoBeep = false;
      }
    }

    async function salvarParametrosWeb() {
      try {
        const tot = document.getElementById('cfgTotLeituras').value;
        const maxL = document.getElementById('cfgMaxLeituras').value;
        const intL = document.getElementById('cfgIntLeitura').value;
        const intT = document.getElementById('cfgIntTurso').value;

        const body = `tot_leituras=${tot}&max_leituras=${maxL}&int_leitura=${intL}&int_turso=${intT}`;
        const res = await fetch('/config_parametros', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: body
        });
        if (res.ok) {
          const msgEl = document.getElementById('msgSalvarParam');
          msgEl.innerText = "Configurações salvas com sucesso no ESP32!";
          setTimeout(() => { msgEl.innerText = ""; }, 4000);
        }
      } catch (err) {
        console.error("Erro ao salvar parametros:", err);
      }
    }

    async function atualizarDados() {
      try {
        const response = await fetch('/dados');
        const dados = await response.json();

        // Sincroniza estado da checkbox do Beep e dos Campos de Configuração
        if (dados.beep_ativo !== undefined && !usuarioAlterandoBeep) {
          document.getElementById('chkBeep').checked = dados.beep_ativo;
        }

        if (!usuarioEditandoCampos) {
          if (dados.total_leituras !== undefined) document.getElementById('cfgTotLeituras').value = dados.total_leituras;
          if (dados.max_leituras !== undefined) document.getElementById('cfgMaxLeituras').value = dados.max_leituras;
          if (dados.intervalo_leitura_s !== undefined) document.getElementById('cfgIntLeitura').value = dados.intervalo_leitura_s;
          if (dados.intervalo_turso_m !== undefined) document.getElementById('cfgIntTurso').value = dados.intervalo_turso_m;
        }

        // Atualiza cards
        const pct = dados.nivel_pct !== undefined ? dados.nivel_pct.toFixed(1) : 0;
        const dist = dados.distancia_cm !== undefined ? dados.distancia_cm.toFixed(1) : 0;

        document.getElementById('valPct').innerText = `${pct} %`;
        document.getElementById('valDist').innerText = `${dist} cm`;

        // Atualiza barra visual de nível
        const fillEl = document.getElementById('tankFill');
        const textEl = document.getElementById('tankText');
        const clampedPct = Math.max(0, Math.min(100, pct));
        fillEl.style.width = `${clampedPct}%`;
        textEl.innerText = `${pct}% Preenchido`;

        // Atualiza Badge de Status
        const badgeEl = document.getElementById('statusBadge');
        if (clampedPct < 20) {
          badgeEl.innerHTML = `<span class="status-badge status-danger">NÍVEL CRÍTICO</span>`;
          fillEl.style.background = 'linear-gradient(90deg, #dc2626, #ef4444)';
        } else if (clampedPct < 40) {
          badgeEl.innerHTML = `<span class="status-badge status-warn">NÍVEL BAÍXO</span>`;
          fillEl.style.background = 'linear-gradient(90deg, #ca8a04, #eab308)';
        } else {
          badgeEl.innerHTML = `<span class="status-badge status-ok">NÍVEL NORMAL</span>`;
          fillEl.style.background = 'linear-gradient(90deg, #0284c7, #38bdf8)';
        }

        // Histórico para o gráfico
        const historico = dados.historico_cm || [];
        const labels = historico.map((_, index) => `#${index + 1}`);

        if (!meuGrafico) {
          const ctx = document.getElementById('graficoNivel').getContext('2d');
          meuGrafico = new Chart(ctx, {
            type: 'line',
            data: {
              labels: labels,
              datasets: [{
                label: 'Distância (cm)',
                data: historico,
                borderColor: '#38bdf8',
                backgroundColor: 'rgba(56, 189, 248, 0.15)',
                borderWidth: 2,
                fill: true,
                tension: 0.3
              }]
            },
            options: {
              animation: false,
              responsive: true,
              scales: {
                y: {
                  grid: { color: '#334155' },
                  ticks: { color: '#94a3b8' },
                  title: { display: true, text: 'Distância (cm)', color: '#94a3b8' }
                },
                x: {
                  grid: { color: '#334155' },
                  ticks: { color: '#94a3b8' }
                }
              },
              plugins: {
                legend: { labels: { color: '#f8fafc' } }
              }
            }
          });
        } else {
          meuGrafico.data.labels = labels;
          meuGrafico.data.datasets[0].data = historico;
          meuGrafico.update();
        }
      } catch (error) {
        console.error("Erro ao buscar dados do ESP32:", error);
      }
    }

    window.onload = () => {
      atualizarDados();
      setInterval(atualizarDados, 5000);
    };
  </script>
</body>
</html>
)=====";

#endif