const PREFIX = "runner_NguyenVanCuong";   // PHẢI trùng với code ESP32
const client = mqtt.connect("wss://broker.hivemq.com:8884/mqtt");

client.on("connect", () => {
  document.getElementById("status").textContent = "Đã kết nối";
  client.subscribe(PREFIX + "/data");
});
client.on("error", () => {
  document.getElementById("status").textContent = "Lỗi kết nối";
});

// ----- Biểu đồ -----
function makeChart(id, label, color) {
  return new Chart(document.getElementById(id), {
    type: "line",
    data: { labels: [], datasets: [{ label, data: [], borderColor: color, tension: 0.3 }] },
    options: { animation: false }
  });
}
const chartTemp = makeChart("chartTemp", "Nhiệt độ (°C)", "#e53935");
const chartAlt  = makeChart("chartAlt", "Độ cao (m)", "#1e88e5");

function pushPoint(chart, value) {
  chart.data.labels.push(new Date().toLocaleTimeString());
  chart.data.datasets[0].data.push(value);
  if (chart.data.labels.length > 30) {
    chart.data.labels.shift();
    chart.data.datasets[0].data.shift();
  }
  chart.update();
}

// ----- Nhận dữ liệu từ ESP32 -----
client.on("message", (topic, payload) => {
  if (topic === PREFIX + "/data") {
    const d = JSON.parse(payload.toString());
    document.querySelector(".temp").textContent = d.temp.toFixed(1);
    document.querySelector(".pressure").textContent = d.pressure.toFixed(1);
    document.querySelector(".altitude").textContent = d.altitude.toFixed(1);
    pushPoint(chartTemp, d.temp);
    pushPoint(chartAlt, d.altitude);
  }
});

// ----- Nút điều khiển -----
function setupButton(id, name, topic) {
  const btn = document.getElementById(id);
  let state = false;
  btn.onclick = () => {
    state = !state;
    client.publish(topic, state ? "ON" : "OFF");
    btn.textContent = name + ": " + (state ? "ON" : "OFF");
    btn.classList.toggle("on", state);
  };
}
setupButton("btnLed", "LED", PREFIX + "/led");
setupButton("btnBuzzer", "LOA", PREFIX + "/buzzer");