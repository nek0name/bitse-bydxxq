import uuid


def test_registration_requires_explicit_confirmation(server):
  phone = '139' + str(uuid.uuid4().int % 100000000).zfill(8)
  request = {'phone': phone, 'allowRegistration': False}
  expected = {'registrationRequired': True, 'phone': phone}
  assert server.rpc('user.login', request) == expected
  # Cancelling leaves no account behind: a repeated attempt still asks to register.
  assert server.rpc('user.login', request) == expected
  admin = server.admin()
  assert server.rpc('admin.users', {'query': phone}, admin) == []

  registered = server.rpc('user.login', {**request, 'allowRegistration': True})
  assert registered['token']
  assert registered['user']['phone'] == phone
  assert server.rpc('user.me', token=registered['token'])['id'] == registered['user']['id']
  returning = server.rpc('user.login', request)
  assert returning['user']['id'] == registered['user']['id']
  assert returning['token']
  assert 'registrationRequired' not in returning
  assert len(server.rpc('admin.users', {'query': phone}, admin)) == 1


def test_registration_validation_and_frozen_account(server):
  phone = '139' + str(uuid.uuid4().int % 100000000).zfill(8)
  for invalid in ('false', 0, None):
    error = server.rpc('user.login', {'phone': phone, 'allowRegistration': invalid}, ok=False)
    assert error['code'] == 'VALIDATION_ERROR'
  assert (
    server.rpc('user.login', {'phone': '123', 'allowRegistration': False}, ok=False)['code']
    == 'VALIDATION_ERROR'
  )
  # The legacy protocol remains usable by existing APKs and other clients.
  profile = server.rpc('user.login', {'phone': phone})['user']
  server.rpc('admin.user.status', {'userId': profile['id'], 'status': 'frozen'}, server.admin())
  error = server.rpc('user.login', {'phone': phone, 'allowRegistration': False}, ok=False)
  assert error['code'] == 'ACCOUNT_FROZEN'
