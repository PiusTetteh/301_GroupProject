# Team Quick Start Guide - Multikernel OS

## 🎯 For the Presentation Day

### Pre-Setup (Do This First)
```bash
# 1. Build everything
cd 301_GroupProject
./build_and_test.sh

# 2. Verify tests pass
./multikernel_test
```

### 5 Minutes Before Presentation

**Start Web Server:**
```bash
python3 web_server.py
```

**Open Browser:**
```
http://localhost:5000
```

**Keep Backup Ready:**
```bash
# In case web fails, use CLI
./multikernel_os
```

---

## 🎤 Presentation Flow (10 min)

1. **Intro** (1 min): "Message-passing multikernel OS, no shared memory"
2. **Live Demo** (5 min): Click "Start System", show real-time monitoring
3. **Technical** (2 min): 8 message types, <20μs latency, NUMA-aware
4. **Metrics** (1 min): 90%+ efficiency, 7.2/8.0 cores utilized
5. **Q&A** (1 min): See PRESENTATION_GUIDE.md for answers

---

## ✅ Quick Checklist

- [ ] Build works (`./build_and_test.sh`)
- [ ] Web server starts (`python3 web_server.py`)
- [ ] Dashboard loads (localhost:5000)
- [ ] CLI backup ready (`./multikernel_os`)
- [ ] Know key numbers: <20μs, 90%+, 7.2/8 cores

---

## 💡 Key Talking Points

- **8 independent OS instances** (one per core)
- **Message-passing** (zero shared memory)
- **NUMA-aware** load balancing
- **Complete implementation** (2,110+ lines)
- **Real metrics** (<20μs latency, 90%+ efficiency)

---

## 🔧 If Something Goes Wrong

**Port in use:** Stop any process on port 5000
**Flask missing:** `pip3 install -r requirements.txt`
**Web fails:** Use CLI backup: `./multikernel_os`
**Panic:** Show code and architecture from README.md

---

## 📚 Essential Reading

1. **PRESENTATION_GUIDE.md** - Detailed demo walkthrough
2. **QUICK_REFERENCE.md** - One-page cheat sheet
3. **PROJECT_SUMMARY.md** - Role assignments & contributions

---

**Remember: You have a complete, tested, working system. Be confident! 🚀**
