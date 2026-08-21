(function (w) {
  var K = w.KONTUR;
  var q = K.tsdQueue.slice();
  var i = 0;
  var phase = "ean";
  var msg = "";
  var msgKind = "";
  var printed = [];

  function $(id) { return document.getElementById(id); }

  function cur() { return q[i]; }

  function skuOf() {
    var c = cur();
    return c ? K.sku(c.ean) : null;
  }

  function paint() {
    var c = cur();
    var s = skuOf();
    $("doc").textContent = c ? c.wave : (printed.length ? q[0] && q[0].wave || "MSK-0821-14" : "—");
    $("pos").textContent = !q.length ? "нет заданий" : (i >= q.length ? q.length + " / " + q.length : (i + 1) + " / " + q.length);
    if (!c) {
      $("stage").innerHTML =
        '<div class="msg ok">Задания линии закрыты. Стикеры ушли на принтер. Можно сдать ТСД.</div>' +
        '<div class="done-list">' + printed.map(function (p) {
          return "<div>" + K.esc(p) + "</div>";
        }).join("") + "</div>";
      return;
    }
    $("cellv").textContent = s.cell;
    $("sku").textContent = s.name + ", р." + s.size;
    $("art").innerHTML = "арт. <b>" + K.esc(s.art) + "</b> · заказ <b>" + K.esc(c.id) + "</b>";
    $("ean").disabled = phase !== "ean";
    $("kiz").disabled = phase !== "kiz";
    $("step-ean").className = "step" + (phase === "ean" ? " on" : "");
    $("step-kiz").className = "step" + (phase === "kiz" ? " on" : "");
    $("ean-chip").setAttribute("data-v", s.ean);
    $("ean-chip").textContent = "подставить " + s.ean;
    $("kiz-chip").setAttribute("data-v", s.kiz);
    $("kiz-chip").textContent = "подставить КИЗ этого заказа";
    $("go").disabled = false;
    $("go").textContent = phase === "ean" ? "пробить ШК" : phase === "kiz" ? "пробить КИЗ" : "следующий заказ";
    var box = $("msg");
    if (msg) {
      box.hidden = false;
      box.className = "msg " + msgKind;
      box.innerHTML = msg;
    } else {
      box.hidden = true;
      box.textContent = "";
    }
    if (phase === "ean") setTimeout(function () { $("ean").focus(); }, 30);
    if (phase === "kiz") setTimeout(function () { $("kiz").focus(); }, 30);
  }

  function push(kind, text) {
    K.pushEvent({ at: Date.now(), who: "iv", kind: kind, text: text });
  }

  function submit() {
    var c = cur();
    var s = skuOf();
    if (!c) return;
    msg = "";
    if (phase === "ean") {
      var ean = $("ean").value.trim();
      if (ean !== s.ean) {
        msg = "ШК не из этого заказа. Нужен " + s.ean + ", ячейка " + s.cell + ".";
        msgKind = "bad";
        paint();
        return;
      }
      phase = "kiz";
      push("ean", "ШК " + ean + " → яч. " + s.cell + " · заказ " + c.id);
      paint();
      return;
    }
    if (phase === "kiz") {
      var kiz = $("kiz").value.trim();
      if (kiz !== s.kiz) {
        msg = "КИЗ не из этой поставки. Стикер не печатаем.";
        msgKind = "bad";
        paint();
        return;
      }
      var order = { id: c.id, ean: s.ean, sticker: K.stickerOf(c.id) };
      var saved = K.store.get();
      var picked = saved.pickedIds || {};
      picked[c.id] = true;
      var log = saved.printLog || [];
      log.unshift({ t: Date.now(), who: "ТСД-03", sticker: order.sticker, prn: "Zebra-МСК-1" });
      K.store.set({ pickedIds: picked, printLog: log.slice(0, 20) });
      push("kiz", "КИЗ принят · " + kiz);
      push("print", "стикер WB " + order.sticker + " отправлен на Zebra-МСК-1");
      printed.push(c.id + " · стикер " + order.sticker);
      msg =
        "Стикер <b>" + K.esc(order.sticker) + "</b> отправлен на принтер Zebra-МСК-1." +
        '<div class="sticker-mini">FBS · WB<br><b>' + K.esc(order.sticker) + "</b>заказ " + K.esc(c.id) + "</div>";
      msgKind = "ok";
      phase = "done";
      $("go").textContent = "следующий заказ";
      paint();
      $("go").disabled = false;
      return;
    }
    if (phase === "done") {
      i += 1;
      phase = "ean";
      msg = "";
      $("ean").value = "";
      $("kiz").value = "";
      paint();
    }
  }

  document.addEventListener("click", function (e) {
    var t = e.target && e.target.closest ? e.target.closest("button, .chip") : e.target;
    if (!t) return;
    if (t.id === "go") submit();
    if (t.classList.contains("chip")) {
      var f = t.getAttribute("data-f");
      var v = t.getAttribute("data-v");
      if (f === "ean" && phase === "ean") { $("ean").value = v; submit(); }
      if (f === "kiz" && phase === "kiz") { $("kiz").value = v; submit(); }
    }
  });
  document.addEventListener("keydown", function (e) {
    if (e.key === "Enter" && (e.target.id === "ean" || e.target.id === "kiz")) {
      e.preventDefault();
      submit();
    }
  });

  $("wh").textContent = K.meta.warehouse;
  paint();
})(window);
