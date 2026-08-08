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
      <div class="card">
        <div class="label">Fila Offline</div>
        <div class="value" id="valPendentes">0</div>
        <div class="subvalue" id="subtextPendentes">Aguardando sincronização</div>
      </div>
      <div class="card">
        <div class="label">Enviadas Turso (7d)</div>
        <div class="value" id="valTurso7d" style="color:var(--accent-green);">0</div>
        <div class="subvalue">Medições sincronizadas na nuvem</div>
      </div>
      <div class="card">
        <div class="label">Salvas Offline (7d)</div>
        <div class="value" id="valOffline7d" style="color:var(--accent-yellow);">0</div>
        <div class="subvalue">Medições guardadas na Flash</div>
      </div>
      <div class="card" id="cardModoTurso" style="border: 1px solid var(--accent);">
        <div class="label">Sample Rate Turso</div>
        <div class="value" id="valModoTurso" style="font-size:1.3rem; margin:12px 0; color:var(--accent);">--</div>
        <div class="subvalue" id="subtextModoTurso">Calculando amostragem...</div>
      </div>
      <div class="card" style="border: 1px dashed var(--accent-yellow);">
        <div class="label">Teste de Sincronização</div>
        <button id="btnSimularFalha" onclick="alternarSimulacaoFalha()" style="margin-top:8px; background:var(--accent-yellow); color:#000; font-weight:bold; border:none; padding:6px 10px; border-radius:6px; cursor:pointer; font-size:0.8rem; width:100%;">
          🧪 Simular Queda do Turso
        </button>
        <button onclick="zerarTelemetriaWeb()" style="margin-top:6px; background:rgba(239, 68, 68, 0.2); color:var(--accent-red); border:1px solid var(--accent-red); font-weight:bold; padding:6px 10px; border-radius:6px; cursor:pointer; font-size:0.8rem; width:100%;">
          🧹 Zerar Contadores e Fila
        </button>
        <div class="subvalue" id="statusSimulacao" style="margin-top:6px; font-size:0.75rem;">Modo Normal (Conectado)</div>
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
      <div style="font-size:0.8rem; color:var(--text-sub); text-align:center; margin-bottom:10px;" id="subtextDerivada">Aguardando 12 leituras locais (2 min)...</div>
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
          <label style="font-size:0.8rem; color:var(--text-sub);">Intervalo Turso Seco (min)</label>
          <input type="number" id="cfgIntTurso" min="1" style="width:100%; padding:8px; border-radius:6px; border:1px solid var(--border); background:#0f172a; color:#fff; margin-top:4px;">
        </div>
        <div>
          <label style="font-size:0.8rem; color:var(--text-sub);">Intervalo Turso Chuva (min)</label>
          <input type="number" id="cfgIntTursoChuva" min="1" style="width:100%; padding:8px; border-radius:6px; border:1px solid var(--border); background:#0f172a; color:#fff; margin-top:4px;">
        </div>
        <div>
          <label style="font-size:0.8rem; color:var(--text-sub);">Limiar Subida Chuva (cm)</label>
          <input type="number" step="0.1" id="cfgLimiarSubida" min="0.1" style="width:100%; padding:8px; border-radius:6px; border:1px solid var(--border); background:#0f172a; color:#fff; margin-top:4px;">
        </div>
        <div>
          <label style="font-size:0.8rem; color:var(--text-sub);">Cooldown Chuva (min)</label>
          <input type="number" id="cfgCooldownChuva" min="1" style="width:100%; padding:8px; border-radius:6px; border:1px solid var(--border); background:#0f172a; color:#fff; margin-top:4px;">
        </div>
        <div style="grid-column: 1 / -1; display:flex; align-items:center; gap:10px; margin-top:5px;">
          <input type="checkbox" id="chkModoDinamico" style="width:18px; height:18px; cursor:pointer; accent-color:#38bdf8;">
          <label for="chkModoDinamico" style="font-size:0.85rem; color:var(--text-main); cursor:pointer; font-weight:bold;">🌧️ Ativar Sample Rate Autônomo (Alternar 1m/5m em chuva)</label>
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

    let estadoSimulacaoFalha = false;

    async function alternarSimulacaoFalha() {
      try {
        const novoEstado = !estadoSimulacaoFalha;
        await fetch('/simular_falha?ativo=' + (novoEstado ? '1' : '0'), { method: 'POST' });
        atualizarUiSimulacao(novoEstado);
      } catch (err) {
        console.error("Erro ao alterar simulacao:", err);
      }
    }

    function atualizarUiSimulacao(ativo) {
      estadoSimulacaoFalha = ativo;
      const btn = document.getElementById('btnSimularFalha');
      const status = document.getElementById('statusSimulacao');
      if (!btn || !status) return;
      if (ativo) {
        btn.style.background = 'var(--accent-red)';
        btn.style.color = '#fff';
        btn.innerText = '🔴 Parar Simulação';
        status.innerText = '⚠️ SIMULANDO OFFLINE (Gravando na Flash)';
        status.style.color = 'var(--accent-red)';
      } else {
        btn.style.background = 'var(--accent-yellow)';
        btn.style.color = '#000';
        btn.innerText = '🧪 Simular Queda do Turso';
        status.innerText = 'Modo Normal (Conectado)';
        status.style.color = 'var(--text-sub)';
      }
    }

    async function zerarTelemetriaWeb() {
      if (!confirm("Deseja realmente zerar a fila offline e todos os contadores de telemetria?")) return;
      try {
        await fetch('/zerar_telemetria', { method: 'POST' });
        atualizarDados();
      } catch (err) {
        console.error("Erro ao zerar telemetria:", err);
      }
    }

    async function salvarParametrosWeb() {
      try {
        const tot = document.getElementById('cfgTotLeituras').value;
        const maxL = document.getElementById('cfgMaxLeituras').value;
        const intL = document.getElementById('cfgIntLeitura').value;
        const intT = document.getElementById('cfgIntTurso').value;
        const intTChv = document.getElementById('cfgIntTursoChuva').value;
        const limiar = document.getElementById('cfgLimiarSubida').value;
        const cooldown = document.getElementById('cfgCooldownChuva').value;
        const modoDin = document.getElementById('chkModoDinamico').checked ? '1' : '0';

        const body = `tot_leituras=${tot}&max_leituras=${maxL}&int_leitura=${intL}&int_turso=${intT}&int_turso_chuva=${intTChv}&limiar_subida=${limiar}&cooldown_chuva=${cooldown}&modo_dinamico=${modoDin}`;
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
          if (dados.intervalo_turso_seco_m !== undefined) document.getElementById('cfgIntTurso').value = dados.intervalo_turso_seco_m;
          if (dados.intervalo_turso_chuva_m !== undefined) document.getElementById('cfgIntTursoChuva').value = dados.intervalo_turso_chuva_m;
          if (dados.limiar_subida_cm !== undefined) document.getElementById('cfgLimiarSubida').value = dados.limiar_subida_cm;
          if (dados.cooldown_chuva_m !== undefined) document.getElementById('cfgCooldownChuva').value = dados.cooldown_chuva_m;
          if (dados.modo_dinamico_ativo !== undefined) document.getElementById('chkModoDinamico').checked = dados.modo_dinamico_ativo;
        }

        // Atualiza cards
        const pct = dados.nivel_pct !== undefined ? dados.nivel_pct.toFixed(1) : 0;
        const dist = dados.distancia_cm !== undefined ? dados.distancia_cm.toFixed(1) : 0;

        document.getElementById('valPct').innerText = `${pct} %`;
        document.getElementById('valDist').innerText = `${dist} cm`;

        const pendentes = dados.pendentes_offline !== undefined ? dados.pendentes_offline : 0;
        const totalOffline = dados.total_salvas_offline !== undefined ? dados.total_salvas_offline : 0;
        const totalTurso = dados.total_sincronizadas_turso !== undefined ? dados.total_sincronizadas_turso : 0;

        document.getElementById('valPendentes').innerText = pendentes;
        document.getElementById('subtextPendentes').innerText = pendentes > 0 ? "Aguardando reconexão Turso..." : "Tudo sincronizado";
        document.getElementById('valTurso7d').innerText = totalTurso;
        document.getElementById('valOffline7d').innerText = totalOffline;

        // Atualiza Card de Sample Rate Turso
        const valModoTurso = document.getElementById('valModoTurso');
        const subtextModoTurso = document.getElementById('subtextModoTurso');
        const cardModoTurso = document.getElementById('cardModoTurso');

        if (dados.modo_chuva_ativo) {
          const restS = dados.tempo_restante_cooldown_s || 0;
          const minR = Math.floor(restS / 60);
          const segR = restS % 60;
          const tipo = dados.tipo_evento_dinamico || 1;
          if (tipo === 2) {
            valModoTurso.innerText = `⚡ Descida (${dados.intervalo_turso_chuva_m || 1} min)`;
            valModoTurso.style.color = '#ef4444';
            subtextModoTurso.innerText = `Bomba/Consumo • Cooldown: ${minR}m ${segR}s`;
            cardModoTurso.style.borderColor = '#ef4444';
          } else {
            valModoTurso.innerText = `🌧️ Subida (${dados.intervalo_turso_chuva_m || 1} min)`;
            valModoTurso.style.color = 'var(--accent-yellow)';
            subtextModoTurso.innerText = `Chuva/Enxurrada • Cooldown: ${minR}m ${segR}s`;
            cardModoTurso.style.borderColor = 'var(--accent-yellow)';
          }
        } else if (dados.modo_dinamico_ativo) {
          valModoTurso.innerText = `☀️ Normal (${dados.intervalo_turso_seco_m || 5} min)`;
          valModoTurso.style.color = 'var(--accent-green)';
          subtextModoTurso.innerText = `Monitorando variações >= ${dados.limiar_subida_cm || 1} cm`;
          cardModoTurso.style.borderColor = 'var(--border)';
        } else {
          valModoTurso.innerText = `⏱️ Fixo (${dados.intervalo_turso_m || 5} min)`;
          valModoTurso.style.color = 'var(--text-sub)';
          subtextModoTurso.innerText = `Modo autônomo desativado`;
          cardModoTurso.style.borderColor = 'var(--border)';
        }

        if (dados.simular_falha !== undefined) {
          atualizarUiSimulacao(dados.simular_falha);
        }

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

        // Histórico para o gráfico e cálculo das médias da derivada
        const historico = dados.historico_cm || [];
        const labels = historico.map((_, index) => `#${index + 1}`);

        // Média Recente (últimos 6 pontos = 1 min)
        const mediaRecenteArr = historico.map((val, idx, arr) => {
          if (idx < 5) return null;
          let soma = 0, count = 0;
          for (let k = idx - 5; k <= idx; k++) {
            if (arr[k] > 0) { soma += arr[k]; count++; }
          }
          return count >= 4 ? parseFloat((soma / count).toFixed(1)) : null;
        });

        // Média Passada (6 pontos anteriores = 1-2 min atrás)
        const mediaPassadaArr = historico.map((val, idx, arr) => {
          if (idx < 11) return null;
          let soma = 0, count = 0;
          for (let k = idx - 11; k <= idx - 6; k++) {
            if (arr[k] > 0) { soma += arr[k]; count++; }
          }
          return count >= 4 ? parseFloat((soma / count).toFixed(1)) : null;
        });

        // Atualiza subtexto da derivada acima do gráfico
        const subtextDeriv = document.getElementById('subtextDerivada');
        if (subtextDeriv && dados.media_recente_cm !== undefined && dados.media_passada_cm !== undefined) {
          if (dados.media_recente_cm > 0 && dados.media_passada_cm > 0) {
            const variacao = (dados.media_passada_cm - dados.media_recente_cm).toFixed(1);
            const rotulo = variacao >= 0 ? `Subida: +${variacao}` : `Descida: ${variacao}`;
            const corRotulo = variacao >= 0 ? '#38bdf8' : '#ef4444';
            subtextDeriv.innerHTML = `Média Passada (1-2m): <b style="color:#eab308;">${dados.media_passada_cm.toFixed(1)} cm</b> &bull; Média Recente (1m): <b style="color:#22c55e;">${dados.media_recente_cm.toFixed(1)} cm</b> &bull; Variação: <b style="color:${corRotulo};">${rotulo} cm</b>`;
          } else {
            subtextDeriv.innerText = "Aguardando 12 leituras locais (2 min) para calcular a derivada...";
          }
        }

        if (!meuGrafico) {
          const ctx = document.getElementById('graficoNivel').getContext('2d');
          meuGrafico = new Chart(ctx, {
            type: 'line',
            data: {
              labels: labels,
              datasets: [
                {
                  label: 'Leitura Instantânea (cm)',
                  data: historico,
                  borderColor: 'rgba(148, 163, 184, 0.35)',
                  backgroundColor: 'transparent',
                  borderWidth: 1.5,
                  borderDash: [2, 2],
                  pointRadius: 2,
                  fill: false
                },
                {
                  label: 'Média Recente (1 min)',
                  data: mediaRecenteArr,
                  borderColor: '#22c55e',
                  backgroundColor: 'rgba(34, 197, 94, 0.08)',
                  borderWidth: 2.5,
                  pointRadius: 3,
                  fill: true,
                  tension: 0.3
                },
                {
                  label: 'Média Passada (1-2 min atrás)',
                  data: mediaPassadaArr,
                  borderColor: '#eab308',
                  backgroundColor: 'transparent',
                  borderWidth: 2,
                  borderDash: [5, 4],
                  pointRadius: 3,
                  fill: false,
                  tension: 0.3
                }
              ]
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
          meuGrafico.data.datasets[1].data = mediaRecenteArr;
          meuGrafico.data.datasets[2].data = mediaPassadaArr;
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