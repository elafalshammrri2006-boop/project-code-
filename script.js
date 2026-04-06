setInterval(() => {
  fetch("/data") // طلب البيانات من الـ ESP32
    .then(response => response.json())
    .then(data => {
      // تحديث القيم في واجهة المستخدم
      document.getElementById("depth").innerText = data.depth.toFixed(1);
      document.getElementById("tilt").innerText = data.tilt.toFixed(1);
      document.getElementById("bat").innerText = data.battery.toFixed(0);

      // منطق التنبيه البصري
      const alertDiv = document.getElementById("alert");
      if(data.depth >= 30) {
        alertDiv.innerHTML = "<span class='alert'>⚠️ DANGER: DEEP WATER</span>";
      } else {
        alertDiv.innerHTML = "";
      }
    })
    .catch(err => console.error("Error fetching data:", err));
}, 1000); // تكرار كل ثانية واحدة