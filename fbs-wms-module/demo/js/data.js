/* Учебные данные стенда. Не боевая база, не чужой бренд. */
(function (w) {
  var K = w.KONTUR = w.KONTUR || {};

  K.meta = {
    name: "КОНТУР",
    warehouse: "Северная",
    host: "vps-demo",
    version: "0.9-стенд",
    user: "Смирнов Д.А.",
    role: "диспетчер",
    notice: "Демо-стенд контура. Данные учебные. Поднимается на VPS, это не чужой SaaS и не боевая база клиента."
  };

  K.printers = [
    { id: "zebra-msk-1", name: "Zebra-МСК-1", model: "ZD421", queue: 0, ok: true },
    { id: "tsc-spb-2", name: "TSC-СПБ-2", model: "TE200", queue: 0, ok: true },
    { id: "zebra-kzn-1", name: "Zebra-КЗН-1", model: "ZD421", queue: 0, ok: true }
  ];

  K.workers = [
    { id: "iv", name: "Иванов С.П.", tsd: "ТСД-03", line: "линия 2 · МСК", job: "сборка", on: true },
    { id: "pt", name: "Петрова А.М.", tsd: "ТСД-05", line: "линия 2 · МСК", job: "сборка", on: true },
    { id: "nr", name: "Нуриев Р.И.", tsd: "ТСД-04", line: "приёмка", job: "приёмка", on: true },
    { id: "sv", name: "Савельева О.Н.", tsd: "ТСД-01", line: "линия 1 · СПБ", job: "сборка", on: false }
  ];

  K.clients = [
    { id: "ntx", name: "ООО «НордТекс»", dog: "44/26", cells: "A-12-*" },
    { id: "krm", name: "ИП Каримова Л.Р.", dog: "18/26", cells: "B-03-*" },
    { id: "vlg", name: "ООО «ВолгаТрикотаж»", dog: "07/25", cells: "C-07-*" }
  ];

  K.skus = {
    "4600605024117": {
      ean: "4600605024117",
      art: "NTX-FT-BLK-52",
      name: "Футболка Basic черная",
      size: "52",
      client: "ntx",
      cell: "A-12-04",
      kiz: "0104600605024117215Ab8kQ2",
      gtin: "04600605024117"
    },
    "4600605024124": {
      ean: "4600605024124",
      art: "NTX-LG-GR-46",
      name: "Легинсы Thermo графит",
      size: "46",
      client: "ntx",
      cell: "A-12-05",
      kiz: "0104600605024124217Km2nP9",
      gtin: "04600605024124"
    },
    "4600605024131": {
      ean: "4600605024131",
      art: "NTX-SK-W5",
      name: "Носки спорт, 5 пар",
      size: "25",
      client: "ntx",
      cell: "A-12-06",
      kiz: "0104600605024131213Rt6sL1",
      gtin: "04600605024131"
    },
    "4600605024148": {
      ean: "4600605024148",
      art: "NTX-HD-OL-50",
      name: "Худи начес олива",
      size: "50",
      client: "ntx",
      cell: "A-12-04",
      kiz: "0104600605024148219Qw4dE8",
      gtin: "04600605024148"
    },
    "4680217001184": {
      ean: "4680217001184",
      art: "KRM-RN2-42",
      name: "Кроссовки Run-2 черные",
      size: "42",
      client: "krm",
      cell: "B-03-11",
      kiz: "0104680217001184215Zx1aB3",
      gtin: "04680217001184"
    },
    "4680217001191": {
      ean: "4680217001191",
      art: "KRM-BT-39",
      name: "Ботинки зимние",
      size: "39",
      client: "krm",
      cell: "B-03-12",
      kiz: "0104680217001191218Yc7hJ4",
      gtin: "04680217001191"
    }
  };

  K.demoCodes = {
    ean: "4600605024117",
    kiz: "0104600605024117215Ab8kQ2",
    ean2: "4600605024124",
    kiz2: "0104600605024124217Km2nP9"
  };

  function sku(ean) { return K.skus[ean]; }

  K.stock = [
    { ean: "4600605024117", on: 48, inb: 12, res: 9, avail: 39 },
    { ean: "4600605024124", on: 22, inb: 8, res: 4, avail: 18 },
    { ean: "4600605024131", on: 120, inb: 0, res: 16, avail: 104 },
    { ean: "4600605024148", on: 15, inb: 20, res: 3, avail: 12 },
    { ean: "4680217001184", on: 7, inb: 0, res: 2, avail: 5 },
    { ean: "4680217001191", on: 4, inb: 6, res: 1, avail: 3 }
  ];

  K.receiving = {
    id: "П-1082",
    client: "ntx",
    accepted: 84,
    total: 120,
    started: "16:20",
    last: { ean: "4600605024117", qty: 6, cell: "A-12-04", who: "Нуриев Р.И.", tsd: "ТСД-04" },
    lastAt: Date.now() - 32000
  };

  var mskOrders = [
    { id: "3541287654301", ean: "4600605024117", st: "picked", who: "iv", kiz: true, sticker: "364 512 847" },
    { id: "3541287654302", ean: "4600605024124", st: "picked", who: "pt", kiz: true, sticker: "364 512 848" },
    { id: "3541287654303", ean: "4600605024117", st: "pick", who: "iv", kiz: false, sticker: "364 512 849" },
    { id: "3541287654304", ean: "4600605024148", st: "err", who: "pt", kiz: false, sticker: "364 512 850", err: "КИЗ не из этой поставки" },
    { id: "3541287654305", ean: "4600605024131", st: "picked", who: "iv", kiz: true, sticker: "364 512 851" },
    { id: "3541287654306", ean: "4600605024124", st: "pick", who: "pt", kiz: false, sticker: "364 512 852" },
    { id: "3541287654307", ean: "4600605024117", st: "picked", who: "iv", kiz: true, sticker: "364 512 853" },
    { id: "3541287654308", ean: "4600605024131", st: "wait", who: "", kiz: false, sticker: "364 512 854" },
    { id: "3541287654309", ean: "4680217001184", st: "picked", who: "pt", kiz: true, sticker: "364 512 855" },
    { id: "3541287654310", ean: "4600605024148", st: "pick", who: "iv", kiz: false, sticker: "364 512 856" },
    { id: "3541287654311", ean: "4600605024117", st: "wait", who: "", kiz: false, sticker: "364 512 857" },
    { id: "3541287654312", ean: "4600605024124", st: "picked", who: "pt", kiz: true, sticker: "364 512 858" }
  ];

  var spbOrders = [
    { id: "3619022100101", ean: "4600605024117", st: "picked", who: "sv", kiz: true, sticker: "371 220 101" },
    { id: "3619022100102", ean: "4600605024131", st: "picked", who: "sv", kiz: true, sticker: "371 220 102" },
    { id: "3619022100103", ean: "4600605024124", st: "picked", who: "sv", kiz: true, sticker: "371 220 103" },
    { id: "3619022100104", ean: "4600605024148", st: "picked", who: "sv", kiz: true, sticker: "371 220 104" },
    { id: "3619022100105", ean: "4680217001191", st: "picked", who: "sv", kiz: true, sticker: "371 220 105" },
    { id: "3619022100106", ean: "4600605024117", st: "picked", who: "sv", kiz: true, sticker: "371 220 106" },
    { id: "3619022100107", ean: "4600605024131", st: "picked", who: "sv", kiz: true, sticker: "371 220 107" },
    { id: "3619022100108", ean: "4600605024124", st: "picked", who: "sv", kiz: true, sticker: "371 220 108" }
  ];

  var kznOrders = [
    { id: "3487711002201", ean: "4600605024117", st: "picked", who: "", kiz: true, sticker: "348 771 201" },
    { id: "3487711002202", ean: "4600605024131", st: "wait", who: "", kiz: false, sticker: "348 771 202" },
    { id: "3487711002203", ean: "4680217001184", st: "wait", who: "", kiz: false, sticker: "348 771 203" },
    { id: "3487711002204", ean: "4600605024148", st: "pick", who: "", kiz: false, sticker: "348 771 204" }
  ];

  function enrich(list) {
    return list.map(function (o) {
      var s = sku(o.ean);
      return {
        id: o.id,
        ean: o.ean,
        art: s.art,
        name: s.name,
        size: s.size,
        cell: s.cell,
        client: s.client,
        st: o.st,
        who: o.who,
        kiz: o.kiz,
        kizCode: o.kiz ? s.kiz : "",
        sticker: o.sticker,
        err: o.err || ""
      };
    });
  }

  K.waves = [
    {
      id: "MSK-0821-14",
      city: "МСК",
      cityName: "Москва",
      supply: "WB-MSK-140821",
      created: "14:08",
      total: 48,
      shown: enrich(mskOrders),
      picked: 32,
      err: 1,
      st: "work",
      tsd: ["ТСД-03", "ТСД-05"],
      printer: "zebra-msk-1"
    },
    {
      id: "SPB-0821-07",
      city: "СПБ",
      cityName: "Санкт-Петербург",
      supply: "WB-SPB-8821",
      created: "11:40",
      total: 31,
      shown: enrich(spbOrders),
      picked: 31,
      err: 0,
      st: "ready",
      tsd: ["ТСД-01"],
      printer: "tsc-spb-2"
    },
    {
      id: "KZN-0821-03",
      city: "КЗН",
      cityName: "Казань",
      supply: "WB-KZN-0321",
      created: "16:55",
      total: 26,
      shown: enrich(kznOrders),
      picked: 3,
      err: 0,
      st: "gather",
      tsd: [],
      printer: "zebra-kzn-1"
    },
    {
      id: "MSK-0821-13",
      city: "МСК",
      cityName: "Москва",
      supply: "WB-MSK-130821",
      created: "09:12",
      total: 41,
      shown: [],
      picked: 41,
      err: 0,
      st: "sent",
      sentAt: "12:47",
      tsd: ["ТСД-03"],
      printer: "zebra-msk-1"
    }
  ];

  K.tsdQueue = [
    { wave: "MSK-0821-14", id: "3541287654303", ean: "4600605024117" },
    { wave: "MSK-0821-14", id: "3541287654306", ean: "4600605024124" },
    { wave: "MSK-0821-14", id: "3541287654310", ean: "4600605024148" }
  ];

  K.seedFeed = [
    { t: -38, who: "iv", kind: "ean", text: "ШК 4600605024117 → яч. A-12-04 · заказ 3541287654301" },
    { t: -32, who: "iv", kind: "kiz", text: "КИЗ принят · 0104600605024117215Ab8kQ2" },
    { t: -30, who: "iv", kind: "print", text: "стикер WB 364 512 847 отправлен на Zebra-МСК-1" },
    { t: -24, who: "pt", kind: "ean", text: "ШК 4600605024124 → яч. A-12-05 · заказ 3541287654302" },
    { t: -18, who: "pt", kind: "kiz", text: "КИЗ принят · 0104600605024124217Km2nP9" },
    { t: -16, who: "pt", kind: "print", text: "стикер WB 364 512 848 отправлен на Zebra-МСК-1" },
    { t: -12, who: "pt", kind: "bad", text: "ошибка: КИЗ не из этой поставки · заказ 3541287654304" },
    { t: -8, who: "nr", kind: "in", text: "приёмка П-1082 · +6 шт 4600605024117 → A-12-04" },
    { t: -3, who: "iv", kind: "ean", text: "ШК 4600605024117 → яч. A-12-04 · заказ 3541287654303 · сборка" }
  ];

  K.livePool = [
    { who: "iv", kind: "kiz", text: "КИЗ принят · заказ 3541287654303" },
    { who: "iv", kind: "print", text: "стикер WB 364 512 849 отправлен на Zebra-МСК-1" },
    { who: "nr", kind: "in", text: "приёмка П-1082 · +4 шт 4600605024148 → A-12-04", ean: "4600605024148", qty: 4, cell: "A-12-04" },
    { who: "pt", kind: "ean", text: "ШК 4600605024124 → яч. A-12-05 · заказ 3541287654306" },
    { who: "pt", kind: "kiz", text: "КИЗ принят · заказ 3541287654306" },
    { who: "pt", kind: "print", text: "стикер WB 364 512 852 отправлен на Zebra-МСК-1" },
    { who: "iv", kind: "ean", text: "ШК 4600605024148 → яч. A-12-04 · заказ 3541287654310" },
    { who: "nr", kind: "in", text: "приёмка П-1082 · коробка 18/24 закрыта по яч. A-12-05", ean: "4600605024124", qty: 0, cell: "A-12-05" }
  ];

  K.salesJournal = [
    { t: "15:12", order: "3541008812009", ean: "4600605024131", qty: 1, cell: "A-12-06", src: "WB sale" },
    { t: "14:03", order: "3541008811550", ean: "4600605024117", qty: 1, cell: "A-12-04", src: "WB sale" },
    { t: "11:40", order: "3619007700122", ean: "4600605024124", qty: 1, cell: "A-12-05", src: "WB sale" }
  ];

  K.sku = sku;
  K.clientById = function (id) {
    for (var i = 0; i < K.clients.length; i++) if (K.clients[i].id === id) return K.clients[i];
    return { name: id };
  };
  K.workerById = function (id) {
    for (var i = 0; i < K.workers.length; i++) if (K.workers[i].id === id) return K.workers[i];
    return { name: "—", tsd: "" };
  };
  K.waveById = function (id) {
    for (var i = 0; i < K.waves.length; i++) if (K.waves[i].id === id) return K.waves[i];
    return null;
  };
  K.findByCode = function (code) {
    code = String(code || "").trim();
    if (!code) return null;
    if (K.skus[code]) return { type: "ean", sku: K.skus[code] };
    var k, s;
    for (k in K.skus) {
      s = K.skus[k];
      if (s.kiz === code) return { type: "kiz", sku: s };
    }
    var w, j, o;
    for (w = 0; w < K.waves.length; w++) {
      for (j = 0; j < K.waves[w].shown.length; j++) {
        o = K.waves[w].shown[j];
        if (o.id === code || o.sticker.replace(/\s/g, "") === code.replace(/\s/g, "")) {
          return { type: "order", sku: K.skus[o.ean], order: o, wave: K.waves[w] };
        }
      }
    }
    return null;
  };
})(window);
