function updateVal(id) {
    const el = document.getElementById(id);
    const valEl = document.getElementById(id + '_val');
    let value = el.value;

    // Formatting logic
    if (id === 'misinfo_mult') value += 'x';
    if (id.includes('weight') || id === 'truth_threshold' || id.includes('prob')) {
        value = parseFloat(value).toFixed(2);
    }

    valEl.textContent = value;
}

function showSection(id) {
    document.querySelectorAll('section').forEach(s => s.style.display = 'none');
    const target = document.getElementById(id);
    if (id === 'visualizer') {
        target.style.display = 'flex';
        renderMap(); // Ensure map is rendered when switching
    } else {
        target.style.display = 'grid';
    }

    document.querySelectorAll('.nav-item').forEach(n => n.classList.remove('active'));
    // Use target id for navigation active state
    document.querySelectorAll('.nav-item').forEach(nav => {
        if (nav.getAttribute('onclick').includes(id)) {
            nav.classList.add('active');
        }
    });
}

function generateConfig() {
    const fields = [
        'population', 'timesteps', 'num_towns', 'schools_per_town', 'religious_per_town',
        'school_capacity', 'religious_capacity', 'misinfo_mult',
        'truth_threshold', 'age_weight', 'edu_weight',
        'same_town_weight', 'same_school_weight', 'same_religious_weight',
        'max_conn', 'prob_s_to_e', 'prob_e_to_d',
        'prob_d_to_p', 'prob_d_to_n', 'prob_d_to_r'
    ];

    const mapping = {
        'misinfo_mult': 'misinfo_multiplier',
        'max_conn': 'max_connections'
    };

    let cfgOutput = "# Misinformation Simulation Config\n";
    fields.forEach(id => {
        const el = document.getElementById(id);
        if (el) {
            const key = mapping[id] || id;
            cfgOutput += `${key}=${el.value}\n`;
        }
    });

    const output = document.getElementById('output-preview');
    output.style.display = 'block';
    output.textContent = cfgOutput;

    // Download logic
    const dataStr = "data:text/plain;charset=utf-8," + encodeURIComponent(cfgOutput);
    const downloadAnchorNode = document.createElement('a');
    downloadAnchorNode.setAttribute("href", dataStr);
    downloadAnchorNode.setAttribute("download", "parameters.cfg");
    document.body.appendChild(downloadAnchorNode);
    downloadAnchorNode.click();
    downloadAnchorNode.remove();
}

function resetDefaults() {
    location.reload();
}

// Initialize
document.addEventListener('DOMContentLoaded', () => {
    const allFields = [
        'population', 'timesteps', 'num_towns', 'schools_per_town', 'religious_per_town',
        'school_capacity', 'religious_capacity', 'misinfo_mult',
        'truth_threshold', 'age_weight', 'edu_weight',
        'same_town_weight', 'same_school_weight', 'same_religious_weight',
        'max_conn', 'prob_s_to_e', 'prob_e_to_d',
        'prob_d_to_p', 'prob_d_to_n', 'prob_d_to_r'
    ];
    allFields.forEach(id => {
        if (document.getElementById(id)) updateVal(id);
    });
});

// --- Spatial Visualizer Logic ---
let spatialData = [];
let maxTime = 0;
let currentTime = 0;
let isPlaying = false;
let animationFrameId = null;

function loadSpatialData(input) {
    if (!input.files || !input.files[0]) return;

    const file = input.files[0];
    const reader = new FileReader();

    reader.onload = function (e) {
        const text = e.target.result;
        const rows = text.split('\n').filter(r => r.trim() !== '');

        // Skip header
        spatialData = rows.slice(1).map(row => {
            const cols = row.split(',');
            return {
                time: parseInt(cols[0]),
                agentId: parseInt(cols[1]),
                townId: parseInt(cols[2]),
                schoolId: parseInt(cols[3]),
                religiousId: parseInt(cols[4]),
                claimId: parseInt(cols[5]),
                state: parseInt(cols[6]),
                isMisinfo: parseInt(cols[7]) === 1
            };
        });

        if (spatialData.length > 0) {
            maxTime = Math.max(...spatialData.map(d => d.time));
            document.getElementById('playback-controls').style.display = 'flex';
            document.getElementById('time-slider').max = maxTime;
            currentTime = 0;
            onTimeSliderChange(0);

            const log = document.getElementById('sim-log');
            log.innerHTML = '<div>> Data loaded successfully.</div>';
        }
    };

    reader.readAsText(file);
}

function onTimeSliderChange(val) {
    currentTime = parseInt(val);
    document.getElementById('current-time-display').textContent = `T: ${currentTime}`;
    updateAnalytics();
    renderMap();
}

function updateAnalytics() {
    if (spatialData.length === 0) return;

    const selectedClaim = parseInt(document.getElementById('claim-selector').value);
    const snap = spatialData.filter(d => d.time === currentTime && d.claimId === selectedClaim);

    // Count states
    let propagating = 0;
    let recovered = 0;
    const populationInput = document.getElementById('population');
    let totalAgents = populationInput ? parseInt(populationInput.value) : 1000;

    snap.forEach(d => {
        if (d.state === 3) propagating++;
        if (d.state === 4 || d.state === 5) recovered++;
    });

    const reach = ((recovered / totalAgents) * 100).toFixed(1);

    document.getElementById('stat-propagating').textContent = propagating;
    document.getElementById('stat-recovered').textContent = recovered;
    document.getElementById('stat-reach').textContent = `${reach}%`;

    // Logging significant events
    if (currentTime > 0 && currentTime % 50 === 0) {
        const log = document.getElementById('sim-log');
        const entry = document.createElement('div');
        entry.textContent = `> T:${currentTime}: Reach at ${reach}%`;
        log.prepend(entry);
    }
}

function togglePlayback() {
    isPlaying = !isPlaying;
    const btn = document.getElementById('play-btn');
    btn.textContent = isPlaying ? '⏸ Pause' : '▶ Play';

    if (isPlaying) {
        animate();
    } else {
        cancelAnimationFrame(animationFrameId);
    }
}

function animate() {
    if (!isPlaying) return;

    currentTime++;
    if (currentTime > maxTime) {
        currentTime = 0;
        isPlaying = false;
        const btn = document.getElementById('play-btn');
        if (btn) btn.textContent = '▶ Play';
    } else {
        const slider = document.getElementById('time-slider');
        if (slider) slider.value = currentTime;
        onTimeSliderChange(currentTime);
        animationFrameId = requestAnimationFrame(animate);
    }
}

// Simple helper to generate consistent pseudo-random offsets for agents
function getAgentOffset(agentId) {
    const x = ((agentId * 123.45) % 30) - 15;
    const y = ((agentId * 678.90) % 30) - 15;
    return { x, y };
}

function renderMap() {
    const canvas = document.getElementById('map-canvas');
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    const numTownsInput = document.getElementById('num_towns');
    const numTowns = numTownsInput ? parseInt(numTownsInput.value) : 5;
    const cols = Math.ceil(Math.sqrt(numTowns));
    const rows = Math.ceil(numTowns / cols);
    const cellWidth = canvas.width / cols;
    const cellHeight = canvas.height / rows;

    const selector = document.getElementById('claim-selector');
    const selectedClaim = selector ? parseInt(selector.value) : 0;
    const currentSnapshot = spatialData.filter(d => d.time === currentTime && d.claimId === selectedClaim);

    // Draw Towns and Building Zones
    for (let i = 0; i < numTowns; i++) {
        const r = Math.floor(i / cols);
        const c = i % cols;
        const x = c * cellWidth;
        const y = r * cellHeight;

        // Town Border
        ctx.strokeStyle = 'rgba(255,255,255,0.08)';
        ctx.lineWidth = 1;
        ctx.strokeRect(x + 5, y + 5, cellWidth - 10, cellHeight - 10);

        // Town Label
        ctx.fillStyle = 'rgba(255,255,255,0.2)';
        ctx.font = '10px Inter';
        ctx.fillText(`DISTRICT ${i}`, x + 15, y + 20);

        // School Zone
        ctx.setLineDash([5, 5]);
        ctx.strokeStyle = 'rgba(99, 102, 241, 0.2)';
        ctx.strokeRect(x + 15, y + 30, cellWidth * 0.4, cellHeight * 0.4);

        // Religious Zone
        ctx.strokeStyle = 'rgba(168, 85, 247, 0.2)';
        ctx.strokeRect(x + cellWidth * 0.5, y + cellHeight * 0.5, cellWidth * 0.4, cellHeight * 0.4);
        ctx.setLineDash([]);
    }

    // Draw Agents
    currentSnapshot.forEach(d => {
        const townR = Math.floor(d.townId / cols);
        const townC = d.townId % cols;

        let targetX = (townC + 0.5) * cellWidth;
        let targetY = (townR + 0.5) * cellHeight;

        if (d.schoolId !== -1) {
            targetX = (townC + 0.3) * cellWidth;
            targetY = (townR + 0.4) * cellHeight;
        } else if (d.religiousId !== -1) {
            targetX = (townC + 0.7) * cellWidth;
            targetY = (townR + 0.7) * cellHeight;
        } else {
            // General residential jitter
            targetX = (townC + 0.5) * cellWidth;
            targetY = (townR + 0.3) * cellHeight;
        }

        const offset = getAgentOffset(d.agentId);
        const x = targetX + offset.x;
        const y = targetY + offset.y;

        let color = 'rgba(255, 255, 255, 0.1)';
        let size = 2;
        let glow = false;

        if (d.state === 3) { // Propagating
            color = d.isMisinfo ? '#ef4444' : '#4f46e5';
            size = 4;
            glow = true;
        } else if (d.state === 1 || d.state === 2) { // Exposed or Doubtful
            color = '#f59e0b';
        } else if (d.state === 4 || d.state === 5) { // Adopted / Immune
            color = '#10b981';
            size = 3;
        }

        if (glow) {
            ctx.shadowBlur = 10;
            ctx.shadowColor = color;
        }

        ctx.fillStyle = color;
        ctx.beginPath();
        ctx.arc(x, y, size, 0, Math.PI * 2);
        ctx.fill();

        ctx.shadowBlur = 0; // Reset
    });
}
