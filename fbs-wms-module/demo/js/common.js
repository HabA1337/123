(function (w) {
  var K = w.KONTUR = w.KONTUR || {};

  K.esc = function (s) {
    return String(s == null ? "" : s)
      .replace(/&/g, "&amp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;")
      .replace(/"/g, "&quot;");
  };

  K.hash = function (s) {
    var h = 2166136261;
    s = String(s);
    for (var i = 0; i < s.length; i++) {
      h ^= s.charCodeAt(i);
      h = Math.imul(h, 16777619);
    }
    return h >>> 0;
  };

  K.pad = function (n) {
    return n < 10 ? "0" + n : String(n);
  };

  K.fmtTime = function (d) {
    d = d instanceof Date ? d : new Date(d);
    return K.pad(d.getHours()) + ":" + K.pad(d.getMinutes()) + ":" + K.pad(d.getSeconds());
  };

  K.fmtHm = function (d) {
    d = d instanceof Date ? d : new Date(d);
    return K.pad(d.getHours()) + ":" + K.pad(d.getMinutes());
  };

  K.fmtDate = function (d) {
    d = d instanceof Date ? d : new Date(d);
    return K.pad(d.getDate()) + "." + K.pad(d.getMonth() + 1) + "." + d.getFullYear();
  };

  K.barcodeHTML = function (text, cls) {
    var h = K.hash(text);
    var html = '<div class="bc' + (cls ? " " + cls : "") + '" title="' + K.esc(text) + '">';
    var i, w, bit;
    for (i = 0; i < 56; i++) {
      h = Math.imul(h ^ (i * 131), 16777619);
      bit = (h >>> 8) & 1;
      w = 1 + (h % 3);
      html += '<i style="width:' + w + "px;opacity:" + (bit ? 1 : 0) + '"></i>';
    }
    html += "</div>";
    return html;
  };

  K.dmCanvas = function (text, size) {
    size = size || 72;
    var n = 18;
    var c = document.createElement("canvas");
    c.width = size;
    c.height = size;
    c.className = "dm";
    c.setAttribute("aria-label", "макет DataMatrix");
    var ctx = c.getContext("2d");
    var cell = size / n;
    ctx.fillStyle = "#fff";
    ctx.fillRect(0, 0, size, size);
    ctx.fillStyle = "#111";
    var h = K.hash(text);
    var x, y, on;
    for (y = 0; y < n; y++) {
      for (x = 0; x < n; x++) {
        if (x === 0 || y === n - 1) on = 1;
        else if (y === 0) on = x % 2 === 0 ? 1 : 0;
        else if (x === n - 1) on = y % 2 === 0 ? 1 : 0;
        else {
          h = Math.imul(h ^ (x + 33) * (y + 17), 16777619);
          on = (h >>> 11) & 1;
        }
        if (on) ctx.fillRect(Math.floor(x * cell), Math.floor(y * cell), Math.ceil(cell), Math.ceil(cell));
      }
    }
    return c;
  };

  K.stickerHTML = function (order, sku) {
    sku = sku || K.sku(order.ean);
    return (
      '<div class="sticker">' +
        '<div class="wb"><span>FBS · WB</span><span>стикер поставки</span></div>' +
        K.barcodeHTML(order.sticker || order.id) +
        '<div class="id">' + K.esc(order.sticker || "—") + "</div>" +
        '<div class="meta">заказ ' + K.esc(order.id) + "<br>" +
          K.esc(sku.name) + ", р." + K.esc(sku.size) + "<br>" +
          "арт. " + K.esc(sku.art) + " · яч. " + K.esc(sku.cell) +
        "</div>" +
      "</div>"
    );
  };

  K.stLabel = function (st) {
    if (st === "picked") return '<span class="st st-ok">собран</span>';
    if (st === "pick") return '<span class="st st-work">на сборке</span>';
    if (st === "err") return '<span class="st st-bad">ошибка КИЗ</span>';
    if (st === "wait") return '<span class="st st-wait">в очереди</span>';
    if (st === "sent") return '<span class="st st-gone">в доставке</span>';
    return '<span class="st st-wait">' + K.esc(st) + "</span>";
  };

  K.waveLabel = function (st) {
    if (st === "work") return '<span class="st st-work">сборка</span>';
    if (st === "ready") return '<span class="st st-ok">собрана</span>';
    if (st === "gather") return '<span class="st st-wait">набор волны</span>';
    if (st === "sent") return '<span class="st st-gone">в доставке</span>';
    return K.stLabel(st);
  };

  K.pct = function (wave) {
    if (!wave.total) return 0;
    return Math.round((wave.picked / wave.total) * 100);
  };

  K.store = {
    key: "kontur-demo-v1",
    get: function () {
      try {
        return JSON.parse(localStorage.getItem(K.store.key) || "{}");
      } catch (e) {
        return {};
      }
    },
    set: function (patch) {
      var cur = K.store.get();
      for (var k in patch) cur[k] = patch[k];
      localStorage.setItem(K.store.key, JSON.stringify(cur));
      try {
        w.dispatchEvent(new CustomEvent("kontur-store", { detail: cur }));
      } catch (e2) {}
    }
  };

  K.pushEvent = function (ev) {
    var s = K.store.get();
    var list = s.extraFeed || [];
    list.unshift(ev);
    if (list.length > 80) list = list.slice(0, 80);
    K.store.set({ extraFeed: list });
  };

  K.applySavedWaves = function () {
    var s = K.store.get();
    if (s.delivered && s.delivered.id) {
      var w0 = K.waveById(s.delivered.id);
      if (w0) {
        w0.st = "sent";
        w0.sentAt = s.delivered.at;
        w0.supplyWb = s.delivered.supplyWb;
      }
    }
    if (s.pickedIds) {
      K.waves.forEach(function (wave) {
        wave.shown.forEach(function (o) {
          if (s.pickedIds[o.id]) {
            o.st = "picked";
            o.kiz = true;
            if (!wave.picked || wave.picked < wave.total) {
              /* keep listed pick count stable unless it was in-progress */
            }
          }
        });
      });
    }
    if (typeof s.recv === "number") K.receiving.accepted = s.recv;
    if (s.stock) {
      K.stock.forEach(function (row) {
        if (s.stock[row.ean]) {
          row.on = s.stock[row.ean].on;
          row.res = s.stock[row.ean].res;
          row.avail = s.stock[row.ean].avail;
        }
      });
    }
    if (s.sales) K.salesJournal = s.sales.concat(K.salesJournal.filter(function (x) {
      return !s.sales.some(function (y) { return y.order === x.order && y.t === x.t; });
    }));
  };
})(window);
