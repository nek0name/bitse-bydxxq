import sqlite3
import time
import uuid

import pytest


def stats(server, token):
  return server.rpc('user.me', token=token)['profileStats']


def reserve(server, token):
  return server.rpc('orders.reserve', {'chargerId': 1, 'idempotencyKey': str(uuid.uuid4())}, token)


def test_profile_stats_track_completed_energy_and_actual_payment(server):
  token, profile = server.user(10000)
  empty = {'orderCount': 0, 'totalEnergyKwh': 0, 'totalSpentCents': 0}
  assert profile['profileStats'] == empty
  assert stats(server, token) == empty  # Recharging is not spending.
  cancelled = reserve(server, token)
  assert stats(server, token) == empty
  server.rpc('orders.cancel', {'orderId': cancelled['id']}, token)
  assert stats(server, token) == empty

  order = reserve(server, token)
  params = {'orderId': order['id']}
  server.rpc('orders.start', params, token)
  assert stats(server, token) == empty  # Only ended charging sessions count.
  time.sleep(0.25)
  pending = server.rpc('orders.stop', params, token)
  assert pending['energyKwh'] > 0
  assert stats(server, token) == {
    'orderCount': 1,
    'totalEnergyKwh': pending['energyKwh'],
    'totalSpentCents': 0,
  }
  paid = server.rpc('orders.settle', params, token)
  expected = {
    'orderCount': 1,
    'totalEnergyKwh': paid['energyKwh'],
    'totalSpentCents': paid['amountCents'],
  }
  assert expected['totalSpentCents'] == 10000 - paid['balanceCents']
  assert stats(server, token) == expected
  server.rpc('orders.settle', params, token)
  assert stats(server, token) == expected
  returning = server.rpc('user.login', {'phone': profile['phone']})
  assert returning['user']['profileStats'] == expected
  updated = server.rpc('user.update', {'nickname': '统计测试'}, token)
  assert updated['profileStats'] == expected


def test_profile_stats_cover_all_pages_and_use_only_own_wallet_debits(server):
  token, profile = server.user(100000)
  other, _ = server.user(0)
  with sqlite3.connect(server.directory / 'platform.db') as connection:
    template = connection.execute("SELECT id FROM orders WHERE status='paid' LIMIT 1").fetchone()[0]
    before = 100000
    for index in range(35):
      status = 'paid' if index < 34 else 'pending_payment'
      order_id = connection.execute(
        'INSERT INTO orders (order_no,user_id,station_id,charger_id,station_name,'
        'charger_code,charger_type,power_kw,price_cents,status,created_at,expires_at,'
        'started_at,ended_at,energy_wh,duration_seconds,amount_cents,settled_at) '
        'SELECT ?,?,station_id,charger_id,station_name,charger_code,charger_type,'
        'power_kw,price_cents,?,created_at,expires_at,started_at,ended_at,1250,'
        'duration_seconds,999,settled_at FROM orders WHERE id=?',
        (f'PROFILE-STATS-{index}', profile['id'], status, template),
      ).lastrowid
      if status == 'paid':
        # Prove totals use recorded debits rather than a potentially different quote.
        connection.execute(
          'INSERT INTO wallet_transactions '
          '(user_id,order_id,kind,amount_cents,balance_before,balance_after,created_at) '
          "VALUES (?,?,'charge',-150,?,?,datetime('now'))",
          (profile['id'], order_id, before, before - 150),
        )
        before -= 150
    connection.execute('UPDATE users SET balance_cents=? WHERE id=?', (before, profile['id']))

  first = server.rpc('orders.list', token=token)
  assert len(first['items']) == 30
  assert first['nextCursor']
  summary = stats(server, token)
  assert summary == {'orderCount': 35, 'totalEnergyKwh': 43.75, 'totalSpentCents': 5100}
  assert summary['totalEnergyKwh'] == pytest.approx(35 * 1.25)
  # Both pagination and order filters are independent of the server-wide summary.
  second = server.rpc('orders.list', {'cursor': first['nextCursor']}, token)
  assert len(second['items']) == 5
  server.rpc('orders.list', {'filter': 'paid', 'limit': 1}, token)
  assert stats(server, token) == summary
  assert stats(server, other) == {'orderCount': 0, 'totalEnergyKwh': 0, 'totalSpentCents': 0}
  assert server.rpc('user.me', {'userId': profile['id']}, other)['profileStats']['orderCount'] == 0
