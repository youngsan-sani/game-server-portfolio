## 개발 사례 - 랭킹 던전

### 개요

나이트워커에서 유저의 데미지 별로 순위 경쟁을 하는 랭킹 던전(데자뷔)을 개발했습니다.
유저를 티어별로 분류하고 리그 내 랭킹 경쟁 및 주간 티어 승강급을 처리합니다.

### 랭킹 구조

- 같은 티어의 유저를 N명씩 무작위로 묶어 리그를 구성
- 유저가 랭킹 던전에서 기록한 데미지 중 Top 3 합산 점수로 리그 내 순위를 결정
- 주 단위로 리그 내 상위 랭커는 티어 승급, 하위 랭커는 티어 강등

### Redis 활용

- 유저별 Top 3 데미지 및 리그 내 순위는 Redis Sorted Set으로 관리
- 주간 정산 시 티어별 유저 풀은 Redis Set으로 관리
- 주간 정산 처리 시 Redis 분산 락으로 단일 서버에만 처리 권한 부여

### Redis 스크립트 샘플

**분산 락 획득**

정산 처리 권한을 획득한 서버에서만 정산이 실행되도록 분산 락을 구현했습니다.
```lua
local get_reply = redis.call('SET', 'ranking_dungeon:result:lock', 'LOCKED', 'NX', 'EX', 300)
if get_reply ~= false then
    return 'LOCK'
end
return redis.call('GET', 'ranking_dungeon:result:lock')
```

**티어별 유저 리그 분배**

티어별 유저 풀에서 n명씩 꺼내 리그를 구성합니다.
- 마지막 리그는 n명 미만이 될 수 있지만 최소 인원보다는 적지 않게 하기위해 n명보다 많은 인원(max 이하)으로 구성될 수 있습니다.
- (n = defaultLeagueSize, max = maxLeagueSize, 최소 인원 = maxLeagueSize - defaultLeagueSize)
```lua
local nowLeague = 1
local defaultLeagueSize = ARGV[1]
local maxLeagueSize = ARGV[2]
local remainCount = tonumber(redis.call('SCARD', 'ranking_dungeon:league:pool'))
local result = 0
while remainCount > 0 do
    local popCount = defaultLeagueSize
    if remainCount < tonumber(maxLeagueSize) then
        popCount = remainCount
    end
    local popList = redis.call('SPOP', 'ranking_dungeon:league:pool', popCount)
    for i = 1, #popList do
        local rankingKey = 'ranking_dungeon:league:' .. nowLeague
        redis.call('ZADD', rankingKey, 0, popList[i])
        redis.call('EXPIRE', rankingKey, 1209600)
    end
    remainCount = remainCount - popCount
    if remainCount > 0 then
        nowLeague = nowLeague + 1
    end
end
return tonumber(nowLeague)
```

### 사용 기술

- C++, MySQL, Redis
